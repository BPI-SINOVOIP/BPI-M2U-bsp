
TARGET_TMP=$PWD"/target_tmp"

cleanup() {
	sudo umount $TARGET_TMP || true
	sudo sudo rm -rf $TARGET_TMP
}

die() {
	echo "$*" >&2
	cleanup
	exit 1
}

set -e

make_rootfs()
{
	echo "Make rootfs"
	local rootfs=$(readlink -f "$1")
	local fsizeinbytes=$(gzip -lq "$rootfs" | awk -F" " '{print $2}')
	local fsizeMB=$(expr $fsizeinbytes / 1024 / 1024 + 200)

	echo "Make rootfs.ext4 (size="$fsizeMB")"
	mkdir -p $TARGET_TMP
	rm -f rootfs.ext4
	dd if=/dev/zero of=rootfs.ext4 bs=1M count="$fsizeMB"
	mkfs.ext4 -q -F rootfs.ext4
	sudo umount $TARGET_TMP || true
	sudo mount -t ext4 rootfs.ext4 $TARGET_TMP

	cd $TARGET_TMP
	echo "Unpacking $rootfs"
	sudo tar xzpf $rootfs || die "Unable to extract rootfs"

	#for x in '' \
	#	'binary/boot/filesystem.dir' 'binary'; do

	#	d="$TARGET${x:+/$x}"

	#	if [ -d "$d/sbin" ]; then
	#		rootfs_copied=1
	#		sudo cp -r "$d"/* $TARGET ||
	#			die "Failed to copy rootfs data"
	#		break
	#	fi
	#done

	#[ -n "$rootfs_copied" ] || die "Unsupported rootfs"

	cd - > /dev/null

	#echo "output = $output"

	#mv linux.ext4 $output
}

[ $# -eq 1 ] || die "Usage: $0 [rootfs.tar.gz]"

echo "s1 = $1"

make_rootfs "$1" 
cleanup

