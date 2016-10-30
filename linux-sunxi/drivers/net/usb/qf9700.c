/*
 * QF9700 one chip USB 1.1 ethernet devices
 *
 * Author : jokeliujl <jokeliu@163.com>
 * Date : 2010-10-01
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

//#define DEBUG

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/usb/usbnet.h>

#include "qf9700.h"

static const char driver_name[] = "qf9700";
/* ------------------------------------------------------------------------------------------ */
/* qf9700 mac and phy operations */
/* qf9700 read some registers from MAC */
static int qf_read(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	void *buf;
	int err = -ENOMEM;

	netdev_dbg(dev->net, "qf_read() reg=0x%02x length=%d", reg, length);

	buf = kmalloc(length, GFP_KERNEL);
	if (!buf)
		goto out;

	err = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 
				QF_RD_REGS, QF_REQ_RD_REG,
			    0, reg, buf, length, USB_CTRL_SET_TIMEOUT);
	if (err == length)
		memcpy(data, buf, length);
	else if (err >= 0)
		err = -EINVAL;
	kfree(buf);

 out:
	return err;
}

/* qf9700 write some registers to MAC */
static int qf_write(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	void *buf = NULL;
	int err = -ENOMEM;

	netdev_dbg(dev->net, "qf_write() reg=0x%02x, length=%d", reg, length);

	if (data) {
		buf = kmemdup(data, length, GFP_KERNEL);
		if (!buf)
			goto out;
	}

	err = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			      QF_WR_REGS, QF_REQ_WR_REG,
			      0, reg, buf, length, USB_CTRL_SET_TIMEOUT);
	kfree(buf);
	if (err >= 0 && err < length)
		err = -EINVAL;
 out:
	return err;
}

/* qf9700 read one register from MAC */
static int qf_read_reg(struct usbnet *dev, u8 reg, u8 *value)
{
	return qf_read(dev, reg, 1, value);
}

/* qf9700 write one register to MAC */
static int qf_write_reg(struct usbnet *dev, u8 reg, u8 value)
{
	netdev_dbg(dev->net, "qf_write_reg() reg=0x%02x, value=0x%02x", reg, value);
	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			       QF_WR_REG, QF_REQ_WR_REG,
			       value, reg, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/* async mode for writing registers or reg blocks */
static void qf_write_async_callback(struct urb *urb)
{
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)urb->context;

	if (urb->status < 0)
		printk(KERN_DEBUG "qf_write_async_callback() failed with %d\n", urb->status);

	kfree(req);
	usb_free_urb(urb);
}

static void qf_write_async_helper(struct usbnet *dev, u8 reg, u8 value, u16 length, void *data)
{
	struct usb_ctrlrequest *req;
	struct urb *urb;
	int status;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		netdev_err(dev->net, "Error allocating URB in qf_write_async_helper!");
		return;
	}

	req = kmalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
	if (!req) {
		netdev_err(dev->net, "Failed to allocate memory for control request");
		usb_free_urb(urb);
		return;
	}

	req->bRequestType = QF_REQ_WR_REG;
	req->bRequest = length ? QF_WR_REGS : QF_WR_REG;
	req->wValue = cpu_to_le16(value);
	req->wIndex = cpu_to_le16(reg);
	req->wLength = cpu_to_le16(length);

	usb_fill_control_urb(urb, dev->udev, usb_sndctrlpipe(dev->udev, 0),
			     (void *)req, data, length,
			     qf_write_async_callback, req);

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status < 0) {
		netdev_err(dev->net, "Error submitting the control message: status=%d",
		       status);
		kfree(req);
		usb_free_urb(urb);
	}

	return;
}

static void qf_write_async(struct usbnet *dev, u8 reg, u16 length, void *data)
{
	netdev_dbg(dev->net, "qf_write_async() reg=0x%02x length=%d", reg, length);

	qf_write_async_helper(dev, reg, 0, length, data);
}

static void qf_write_reg_async(struct usbnet *dev, u8 reg, u8 value)
{
	netdev_dbg(dev->net, "qf_write_reg_async() reg=0x%02x value=0x%02x", reg, value);

	qf_write_async_helper(dev, reg, value, 0, NULL);
}

/* qf9700 read one word from phy or eeprom  */
static int qf_share_read_word(struct usbnet *dev, int phy, u8 reg, __le16 *value)
{
	int ret, i;

	mutex_lock(&dev->phy_mutex);

	qf_write_reg(dev, EPAR, phy ? (reg | 0x40) : reg);
	qf_write_reg(dev, EPCR, phy ? 0xc : 0x4);

	for (i = 0; i < QF_SHARE_TIMEOUT; i++) {
		u8 tmp;

		udelay(1);
		ret = qf_read_reg(dev, EPCR, &tmp);
		if (ret < 0)
			goto out;

		/* ready */
		if ((tmp & 1) == 0)
			break;
	}

	if (i >= QF_SHARE_TIMEOUT) {
		netdev_err(dev->net, "%s read timed out!", phy ? "phy" : "eeprom");
		ret = -EIO;
		goto out;
	}

	qf_write_reg(dev, EPCR, 0x0);
	ret = qf_read(dev, EPDR, 2, value);

	netdev_dbg(dev->net, "read shared %d 0x%02x returned 0x%04x, %d",
	       phy, reg, *value, ret);

 out:
	mutex_unlock(&dev->phy_mutex);
	return ret;
}

/* write one word to phy or eeprom */
static int qf_share_write_word(struct usbnet *dev, int phy, u8 reg, __le16 value)
{
	int ret, i;

	mutex_lock(&dev->phy_mutex);

	ret = qf_write(dev, EPDR, 2, &value);
	if (ret < 0)
		goto out;

	qf_write_reg(dev, EPAR, phy ? (reg | 0x40) : reg);
	qf_write_reg(dev, EPCR, phy ? 0x1a : 0x12);

	for (i = 0; i < QF_SHARE_TIMEOUT; i++) {
		u8 tmp;

		udelay(1);
		ret = qf_read_reg(dev, EPCR, &tmp);
		if (ret < 0)
			goto out;

		/* ready */
		if ((tmp & 1) == 0)
			break;
	}

	if (i >= QF_SHARE_TIMEOUT) {
		netdev_err(dev->net, "%s write timed out!", phy ? "phy" : "eeprom");
		ret = -EIO;
		goto out;
	}

	qf_write_reg(dev, EPCR, 0x0);

out:
	mutex_unlock(&dev->phy_mutex);
	return ret;
}

static int qf_read_eeprom_word(struct usbnet *dev, u8 offset, void *value)
{
	return qf_share_read_word(dev, 0, offset, value);
}


static int qf9700_get_eeprom_len(struct net_device *dev)
{
	return QF_EEPROM_LEN;
}

/* get qf9700 eeprom information */
static int qf9700_get_eeprom(struct net_device *net, struct ethtool_eeprom *eeprom, u8 * data)
{
	struct usbnet *dev = netdev_priv(net);
	__le16 *ebuf = (__le16 *) data;
	int i;

	/* access is 16bit */
	if ((eeprom->offset % 2) || (eeprom->len % 2))
		return -EINVAL;

	for (i = 0; i < eeprom->len / 2; i++) {
		if (qf_read_eeprom_word(dev, eeprom->offset / 2 + i, &ebuf[i]) < 0)
			return -EINVAL;
	}
	return 0;
}

/* qf9700 mii-phy register read by word */
static int qf9700_mdio_read(struct net_device *netdev, int phy_id, int loc)
{
	struct usbnet *dev = netdev_priv(netdev);
	__le16 res;

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported");
		return 0;
	}

	qf_share_read_word(dev, 1, loc, &res);

	netdev_dbg(dev->net,
	       "qf9700_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x",
	       phy_id, loc, le16_to_cpu(res));

	return le16_to_cpu(res);
}

/* qf9700 mii-phy register write by word */
static void qf9700_mdio_write(struct net_device *netdev, int phy_id, int loc, int val)
{
	struct usbnet *dev = netdev_priv(netdev);
	__le16 res = cpu_to_le16(val);

	if (phy_id) {
		netdev_dbg(dev->net, "Only internal phy supported");
		return;
	}

	netdev_dbg(dev->net,"qf9700_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x",
	       phy_id, loc, val);

	qf_share_write_word(dev, 1, loc, res);
}

/*-------------------------------------------------------------------------------------------*/

static void qf9700_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	/* Inherit standard device info */
	usbnet_get_drvinfo(net, info);
	strncpy (info->driver, driver_name, sizeof info->driver);
	info->eedump_len = QF_EEPROM_LEN;
}

static u32 qf9700_get_link(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	int rc = 0;
	u8 value = 0;

#if	0
	rc = mii_link_ok(&dev->mii);
#else
	qf_read_reg(dev, NSR, &value);
	if(value & NQF_LINKST) {
		rc = 1;
	}
#endif

	return rc;
}

static int qf9700_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	struct usbnet *dev = netdev_priv(net);

	return generic_mii_ioctl(&dev->mii, if_mii(rq), cmd, NULL);
}

static struct ethtool_ops qf9700_ethtool_ops = {
	.get_drvinfo	= qf9700_get_drvinfo,
	.get_link	= qf9700_get_link,
	.get_msglevel	= usbnet_get_msglevel,
	.set_msglevel	= usbnet_set_msglevel,
	.get_eeprom_len	= qf9700_get_eeprom_len,
	.get_eeprom	= qf9700_get_eeprom,
	.get_settings	= usbnet_get_settings,
	.set_settings	= usbnet_set_settings,
	.nway_reset	= usbnet_nway_reset,
};

static void qf9700_set_multicast(struct net_device *net)
{
	struct usbnet *dev = netdev_priv(net);
	/* We use the 20 byte dev->data for our 8 byte filter buffer
	 * to avoid allocating memory that is tricky to free later */
	u8 *hashes = (u8 *) & dev->data;
	u8 rx_ctl = 0x31;	// enable, disable_long, disable_crc

	memset(hashes, 0x00, QF_MCAST_SIZE);
	hashes[QF_MCAST_SIZE - 1] |= 0x80;	/* broadcast address */

	if (net->flags & IFF_PROMISC) {
		rx_ctl |= 0x02;
	} else if (net->flags & IFF_ALLMULTI ||
			netdev_mc_count(net) > QF_MCAST_MAX) {
		rx_ctl |= 0x04;
	} else if (netdev_mc_count(net)) {
		struct netdev_hw_addr *ha;

		netdev_for_each_mc_addr(ha, net){
			u32 crc = ether_crc(ETH_ALEN, ha->addr) >> 26;
			hashes[crc >> 3] |= 1 << (crc & 0x7);
		}
	}

	qf_write_async(dev, MAR, QF_MCAST_SIZE, hashes);
	qf_write_reg_async(dev, RCR, rx_ctl);
}

static int qf9700_set_mac_address(struct net_device *net, void *p)
{
	struct sockaddr *addr = p;
	struct usbnet *dev = netdev_priv(net);

	if(netif_running(net))
		return -EBUSY;

	if(!is_valid_ether_addr(addr->sa_data)){
		dev_err(&net->dev, "not setting invalid mac address %pM\n", addr->sa_data);
		return -EADDRNOTAVAIL;
	}

	memcpy(net->dev_addr, addr->sa_data, ETH_ALEN);
	qf_write_async(dev, PAR, ETH_ALEN, addr->sa_data);

	return 0;
}

static const struct net_device_ops qf9700_netdev_ops = {
	.ndo_open		= usbnet_open,
	.ndo_stop		= usbnet_stop,
	.ndo_start_xmit		= usbnet_start_xmit,
	.ndo_tx_timeout		= usbnet_tx_timeout,
	.ndo_change_mtu		= usbnet_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= qf9700_ioctl,
	.ndo_set_mac_address 	= qf9700_set_mac_address,
	.ndo_set_rx_mode	= qf9700_set_multicast,
};

static int qf9700_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int ret;

	ret = usbnet_get_endpoints(dev, intf);
	if (ret)
		goto out;

	dev->net->ethtool_ops = &qf9700_ethtool_ops;
	dev->net->netdev_ops  = &qf9700_netdev_ops;

	dev->net->hard_header_len += QF_TX_OVERHEAD;
	dev->hard_mtu = dev->net->mtu + dev->net->hard_header_len;
	dev->rx_urb_size = dev->net->mtu + ETH_HLEN + QF_RX_OVERHEAD;

	dev->mii.dev = dev->net;
	dev->mii.mdio_read = qf9700_mdio_read;
	dev->mii.mdio_write = qf9700_mdio_write;
	dev->mii.phy_id_mask = 0x1f;
	dev->mii.reg_num_mask = 0x1f;

	/* reset the qf9700 */
	qf_write_reg(dev, NCR, 1);
	udelay(20);

	/* read MAC */
	if (qf_read(dev, PAR, ETH_ALEN, dev->net->dev_addr) < 0) {
		printk(KERN_ERR "Error reading MAC address\n");
		ret = -ENODEV;
		goto out;
	}

	/* power up and reset phy */
	qf_write_reg(dev, PRR, 1);
	mdelay(20 );	// at least 10ms, here 20ms for safe
	qf_write_reg(dev, PRR, 0);
	mdelay(2);	// at least 1ms, here 2ms for reading right register

	/* receive broadcast packets */
	qf9700_set_multicast(dev->net);

	qf9700_mdio_write(dev->net, dev->mii.phy_id, MII_BMCR, BMCR_RESET);
	qf9700_mdio_write(dev->net, dev->mii.phy_id, MII_ADVERTISE, ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
	mii_nway_restart(&dev->mii);

out:
	return ret;
}

static int qf9700_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	struct net_device *netdev = dev->net;
	u8 status;
	int len;

	/* format:
	   b0: rx status
	   b1: packet length (incl crc) low
	   b2: packet length (incl crc) high
	   b3..n-4: packet data
	   bn-3..bn: ethernet crc
	 */

	if (unlikely(skb->len < QF_RX_OVERHEAD)) {
		dev_err(&dev->udev->dev, "unexpected tiny rx frame\n");
		return 0;
	}

	status = skb->data[0];
	len = (skb->data[1] | (skb->data[2] << 8)) - 4;

	if (unlikely(status & 0xbf)) {
		if (status & 0x01) netdev->stats.rx_fifo_errors++;
		if (status & 0x02) netdev->stats.rx_crc_errors++;
		if (status & 0x04) netdev->stats.rx_frame_errors++;
		if (status & 0x20) netdev->stats.rx_missed_errors++;
		if (status & 0x90) netdev->stats.rx_length_errors++;
		return 0;
	}

	skb_pull(skb, 3);
	skb_trim(skb, len);

	return 1;
}

static struct sk_buff *qf9700_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	int len;

	/* format:
	   b0: packet length low
	   b1: packet length high
	   b3..n: packet data
	*/

	len = skb->len;

	if (skb_headroom(skb) < QF_TX_OVERHEAD) {
		struct sk_buff *skb2;

		skb2 = skb_copy_expand(skb, QF_TX_OVERHEAD, 0, flags);
		dev_kfree_skb_any(skb);
		skb = skb2;
		if (!skb)
			return NULL;
	}

	__skb_push(skb, QF_TX_OVERHEAD);

	/* usbnet adds padding if length is a multiple of packet size
	   if so, adjust length value in header */
	if ((skb->len % dev->maxpacket) == 0)
		len++;

	skb->data[0] = len;
	skb->data[1] = len >> 8;

	return skb;
}

static void qf9700_status(struct usbnet *dev, struct urb *urb)
{
	int link;
	u8 *buf;

	/* format:
	   b0: net status
	   b1: tx status 1
	   b2: tx status 2
	   b3: rx status
	   b4: rx overflow
	   b5: rx count
	   b6: tx count
	   b7: gpr
	*/

	if (urb->actual_length < 8)
		return;

	buf = urb->transfer_buffer;

	link = !!(buf[0] & 0x40);
	if (netif_carrier_ok(dev->net) != link) {
		if (link) {
			netif_carrier_on(dev->net);
			usbnet_defer_kevent (dev, EVENT_LINK_RESET);
		}else
			netif_carrier_off(dev->net);
		netdev_dbg(dev->net, "Link Status is: %d", link);
	}
}

static int qf9700_link_reset(struct usbnet *dev)
{
	struct ethtool_cmd ecmd;

	mii_check_media(&dev->mii, 1, 1);
	mii_ethtool_gset(&dev->mii, &ecmd);

	netdev_dbg(dev->net, "link_reset() speed: %d duplex: %d",
	       ecmd.speed, ecmd.duplex);

	return 0;
}

static const struct driver_info qf9700_info = {
	.description	= "QF9700 USB Ethernet",
	.flags		= FLAG_ETHER,
	.bind		= qf9700_bind,
	.rx_fixup	= qf9700_rx_fixup,
	.tx_fixup	= qf9700_tx_fixup,
	.status		= qf9700_status,
	.link_reset	= qf9700_link_reset,
	.reset		= qf9700_link_reset,
};

static const struct usb_device_id products[] = {
	{
	 USB_DEVICE(0x0fe6, 0x9700),	/* QF9700 */
	 .driver_info = (unsigned long)&qf9700_info,
	 },
	{},			// END
};

MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver qf9700_driver = {
	.name = "qf9700",
	.id_table = products,
	.probe = usbnet_probe,
	.disconnect = usbnet_disconnect,
	.suspend = usbnet_suspend,
	.resume = usbnet_resume,
};

static int __init qf9700_init(void)
{
	return usb_register(&qf9700_driver);
}

static void __exit qf9700_exit(void)
{
	usb_deregister(&qf9700_driver);
}

module_init(qf9700_init);
module_exit(qf9700_exit);

MODULE_AUTHOR("jokeliu <jokeliu@163.com>");
MODULE_DESCRIPTION("QF9700 one chip USB 2.0 ethernet devices");
MODULE_LICENSE("GPL");
