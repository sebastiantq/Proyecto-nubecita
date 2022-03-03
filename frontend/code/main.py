# Libreria desarrollada por Sebastian Tobar Quintero
# Última modiciación 3/03/2022

import socket
import sys

# Parametros
ip = 'localhost'
port = 10000

def set_ip(new_ip):
    global ip

    ip = new_ip

def set_port(new_port):
    global port

    port = new_port

def connect_socket():
    global ip, port

    return socket.create_connection(ip, port)

# Enviamos una petición al elastic-container-service por medio de un socket
def send_petition(sock, petition):
    try:
        message = str.encode(petition)
        print('Solicitando petición nro. {!r}'.format(message))
        sock.sendall(message)

        amount_received = 0
        amount_expected = len(message)

        while amount_received < amount_expected:
            data = sock.recv(16)
            amount_received += len(data)
            print('Respuesta recibida: {!r}'.format(data))
    finally:
        print('Cerrando socket')
        sock.close()
