from random import randrange
import time
import os

"""
for i in range(50):
    os.system("./client -a 127.0.0.1 -p 8080 -s " + str(randrange(7000)) + " -d " + str(randrange(7000)))

time.sleep(5)
"""

for i in range(1050):
    os.system("./client -a 127.0.0.1 -p 8080 -s " + str(randrange(7000)) + " -d " + str(randrange(7000)))
