/*
 *    Filename: eve_api_include.h
 *     Version: 1.0
 * Description: EVE driver CMD, Don't modify it in user space.
 *     License: GPLv2
 *
 *		Author  : xiongyi <xiongyi@allwinnertech.com>
 *		Date    : 2017/01/9
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef EVE_API_INCLUDE_H
#define EVE_API_INCLUDE_H

#define EVE_MAGIC 'x'

#define EVE_READ_RESNUM			_IO(EVE_MAGIC, 0)
#define EVE_WRITE_REGISTER	_IO(EVE_MAGIC, 1)
#define EVE_READ_REGISTER		_IO(EVE_MAGIC, 2)
#define EVE_OPEN_CLK				_IO(EVE_MAGIC, 3)
#define EVE_CLOSE_CLK				_IO(EVE_MAGIC, 4)
#define EVE_PLL_SET					_IO(EVE_MAGIC, 5)
#define EVE_SYS_RESET				_IO(EVE_MAGIC, 6)
#define EVE_MOD_RESET				_IO(EVE_MAGIC, 7)

struct eve_register {
	unsigned long addr;
	unsigned long value;
};

#endif
