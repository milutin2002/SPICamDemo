import serial
import struct

ser=serial.Serial("/dev/serial0",921600,timeout=1)
n=0
def readImage():
	global n
	magic=ser.read(4)
	MAGIC = b'FRAM'
	while magic!=MAGIC:
		magic=ser.read(4)
		print(magic)
		input("Press key")
	r=input("Waiting for key")
	print("Magic")
	len=struct.unpack('<I',ser.read(4))
	print("Reading data")
	data=ser.read(len)
	fn=f'frame_{n}.jpg'
	with open(fn,'wb') as f:
		f.write(data)
	n+=1

while True:
	readImage()
