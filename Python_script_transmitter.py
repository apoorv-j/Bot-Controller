import sys
import pygame
import time
# from time import sleep
from pygame.locals import *
import serial
import socket


#ser1 = serial.Serial('/dev/cu.usbmodem141401',9600)
SERVER_IP = '192.168.4.1'
SERVER_PORT = 3333
SERVER_ADDRESS = (SERVER_IP, SERVER_PORT)

# global 'sock' variable for socket connection
sock = None


def connect_to_server(SERVER_ADDRESS):
    """
    Purpose:
    ---
    the function creates socket connection with server

    Input Arguments:
    ---
    `SERVER_ADDRESS` :	[ tuple ]
            port address of server

    Returns:
    ---
    `sock` :	[ object of socket class ]
            object of socket class for socket communication

    Example call:
    ---
    sock = connect_to_server(SERVER_ADDRESS)

    """

    global sock
    sock = None

#############  Add your Code here   ###############
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(SERVER_ADDRESS)

###################################################

    return sock

# def send_to_receive_from_server(sock, data_to_send):

# 	"""
# 	Purpose:
# 	---
# 	the function sends / receives data to / from server in proper format

# 	Input Arguments:
# 	---
# 	`sock` :	[ object of socket class ]
# 		object of socket class for socket communication
# 	`string`	:	[ string ]
# 		data to be sent from client to server

# 	Returns:
# 	---
# 	`sent_data` :	[ string ]
# 		data sent from client to server in proper format
# 	`recv_data` :	[ string ]
# 		data sent from server to client in proper format

# 	Example call:
# 	---
# 	sent_data, recv_data = send_to_receive_from_server(sock, shortestPath)

# 	"""

# 	sent_data = ''
# 	recv_data = ''

#############  Add your Code here   ###############

# Send data
    # sent_data = '#' + str(data_to_send) + '#'
    #nsent = sock.send(sent_data.encode())
# nsent = sock.send(str(hello).encode())
# print("Sent %d bytes" % nsent)

    # while(1):
    # 	amount_received = 0
    # 	recv_data = sock.recv(1024)
    # 	recv_data = recv_data.decode()
    # 	amount_received += len(recv_data)
    # 	if(amount_received > 0):
    # 		break

    # print("received data: ", recv_data)
# ###################################################

    # return sent_data, recv_data
size = width, height = 600, 400
black = 0, 0, 0
if __name__ == '__main__':

    sock = connect_to_server(SERVER_ADDRESS)

    pygame.display.init()
    screen = pygame.display.set_mode(size)

    while 1:
        # print("in while")
        for event in pygame.event.get():
            if event.type == KEYDOWN and event.key == K_w:
                print("forward")
                sock.send('w'.encode())
            # if event.type == KEYDOWN and event.key == K_a:
            #     print("HARD left")
            #     sock.send('a'.encode())
            if event.type == KEYDOWN and event.key == K_s:
                print("reverse")
                sock.send('s'.encode())
            # if event.type == KEYDOWN and event.key == K_d:
            #     print("HARD right")
            #     sock.send('d'.encode())
            if event.type == KEYDOWN and event.key == K_a:
                print("SOFT left")
                sock.send('c'.encode())
            if event.type == KEYDOWN and event.key == K_d:
                print("SOFT right")
                sock.send('z'.encode())
            if event.type == KEYDOWN and event.key == K_r:
                print("red led")
                sock.send('r'.encode())
            if event.type == KEYDOWN and event.key == K_g:
                print("green led")
                sock.send('g'.encode())
            if event.type == KEYUP:
                sock.send('.'.encode())
            if event.type == KEYDOWN and event.key == K_ESCAPE:
                sys.exit()
            # time.sleep(0.1)
