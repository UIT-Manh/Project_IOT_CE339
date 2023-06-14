import json
import socket
import threading
import paho.mqtt.client as mqtt

temp = []  
hum = []  
smoke = []  

# Socket server class
class SocketServer(threading.Thread):
    def __init__(self, host, port, mqtt_client):
        super(SocketServer, self).__init__()
        self.host = host
        self.port = port
        self.mqtt_client = mqtt_client 
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        self.server_socket = None
        self.client_sockets = []

    def run(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(1)
        print(f'Socket server listening on {self.host}:{self.port}')
        while True:
            client_socket, client_address = self.server_socket.accept()
            print('New connection:', client_address)
            self.client_sockets.append(client_socket)
            # Handle incoming messages
            while True:
                data = client_socket.recv(1024)
                if not data:
                    break
                message = data.decode().strip()
                print('Received from Socket client:', message)
                try:
                    rev_data = json.loads(message)
                except ValueError:
                    print("Json load error")
                if rev_data['topic'] == "smoke":
                    smoke.append(rev_data['smoke'])
                else:
                    if rev_data['topic'] == "temp-hum":
                        temp.append(rev_data['temperature'])
                        hum.append(rev_data['humidity'])

                if len(smoke) > 0 and len(temp) > 0 and len(hum) > 0:
                    smoke_data = smoke.pop(0)
                    temp_data = temp.pop(0)
                    hum_data = hum.pop(0)
                    env_data = json.dumps({'smoke': smoke_data, 'temp': temp_data, 'hum': hum_data})
                    self.mqtt_client.publish("status/environments", env_data, 1)
                # Do something with the received message (e.g., publish to MQTT)

            # Close the client socket
            client_socket.close()
            
    # Define the MQTT event handlers
    def on_connect(self, client, userdata, flags, rc):
        print('MQTT Connected')
        self.mqtt_client.subscribe('controller/buzzer')  # Subscribe to a topic
        self.mqtt_client.publish("connected", "Rasp", 1)

    def on_message(self, client, userdata, msg):
        print('MQTT Message:', msg.payload.decode())
        if (msg.topic == "controller/buzzer"):
            print('MQTT Message:', msg.topic)
            for client_socket in self.client_sockets:
                data_buzzer = {"topic": "buzzer", "data": msg.payload.decode()}
                data_buzzer_json = json.dumps(data_buzzer)
                client_socket.sendall(data_buzzer_json.encode())
                print(data_buzzer_json)
    def stop(self):
        if self.server_socket:
            self.server_socket.close()

mqtt_client = mqtt.Client()

# Connect to the MQTT broker
mqtt_client.username_pw_set('eXUeRHMZWmaE7CqW9nj2Peio1iKtDrNFdyHS3jGNtGgq6wa6KjYpn5CdbcuCs87v', '')  # Set your MQTT username and password here

mqtt_client.connect("mqtt.flespi.io", 1883, 60)

# Start the Socket server
socket_server = SocketServer('0.0.0.0', 5000, mqtt_client)

socket_server.start()

mqtt_client.loop_start()
