# Importamos nuestro propio paquete
execfile("/nubecita/code/main.py")

main()

sock = connect_socket()

# Escogemos la petición a enviar
send_petition('1')
send_petition('2')
send_petition('3')
send_petition('4')

# Definimos los parametros iniciales
def main():
    set_ip('localhost')
    set_port(2708)
