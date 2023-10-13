# GRUBCommander is a hardware boot select menu powered by Zephyr

To use GRUBCommander, add the following "01_grubcommander" script to your "/etc/grub.d" directory. N.B. you may need to change the boot options to fit your GRUB menu.

```
#! /bin/sh

cat << 'EOF'
# Look for hardware switch device by its hard-coded filesystem ID
search --no-floppy --fs-uuid --set hdswitch 5421-0100

# If found, read dynamic config file and select appropriate entry for each position
if [ "${hdswitch}" ] ; then
  source ($hdswitch)/BOOT_OPT.CFG

  if [ "${os_hw_switch}" == 0 ] ; then
    # Boot Linux
    set default="0"
  elif [ "${os_hw_switch}" == 1 ] ; then
    # Boot Windows
    set default="4"
  fi

fi
EOF
```

Next run:

```
$ sudo update-grub
```
When you twist the rotary encoder on the Adafruit Seesaw, the contents of "BOOT_OPT.CFG" are changed. GRUB sources this file which sets the "os_hw_switch" variable. This is then used to change the GRUB boot select option.
