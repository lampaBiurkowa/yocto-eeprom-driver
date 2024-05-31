simple driver for 24LC512 EEPROM:

echo <addr> <byte> > /proc/dibeeprom : sets value
echo <addr> > /proc/dibeeprom : sets address for read
cat /proc/dibeeprom : reads value
