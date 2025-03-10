#!/sbin/busybox sh
#

# ui_print by Chainfire
OUTFD=$(busybox ps | busybox grep -v "grep" | busybox grep -o -E "/tmp/updater .*" | busybox cut -d " " -f 3);
ui_print() {
  if [ $OUTFD != "" ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    echo "${1}";
  fi;
}

SYSTEM_SIZE='629145600' # 600M
MMC_PART='/dev/block/mmcblk0p1 /dev/block/mmcblk0p2 /dev/block/mmcblk0p3'

# setup lvm volumes
/lvm/sbin/lvm pvcreate $MMC_PART
/lvm/sbin/lvm vgcreate lvpool $MMC_PART
/lvm/sbin/lvm lvcreate -L ${SYSTEM_SIZE}B -n system lvpool
/lvm/sbin/lvm lvcreate -l 100%FREE -n userdata lvpool

# format data (/system will be formatted by updater-script)
make_ext4fs -b 4096 -g 32768 -i 8192 -I 256 -l -16384 -a /data /dev/lvpool/userdata

ui_print "Partitions had been prepared"
