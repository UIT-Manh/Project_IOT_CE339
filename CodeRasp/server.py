import json
import socket
import select
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
        self.sockets_list = []

    def start(self):
        # Tạo socket server
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)

        print(f'Socket server listening on {self.host}:{self.port}')
        # Thêm socket server vào danh sách
        self.sockets_list.append(self.server_socket)

    def run(self):
        while True:
            readable, _, _ = select.select(self.sockets_list, [], [])
            for socket in readable:
                # Xử lý kết nối mới đến
                if socket == self.server_socket:
                    client_socket, client_address = self.server_socket.accept()
                    self.sockets_list.append(client_socket)
                    print("New connect from:", client_address)
                else: 
            # Handle incoming messages
                    data = socket.recv(1024)
                    backup_data = data
                    if not data:
                        print("It not data:", data)
                        print("Close connection from: ", socket.getpeername())
                        socket.close()
                        self.sockets_list.remove(socket)
                    else:
                        message = backup_data.decode().strip()
                        # print('Received from Socket client:', message)
                        try:
                            rev_data = json.loads(message)
                        except ValueError:
                            print("Json load error")
                        if rev_data['topic'] == "smoke":
                            if (rev_data['smoke'] >= 0 and rev_data['smoke'] <= 3200):
                                smoke.append(rev_data['smoke'])
                        else:
                            if rev_data['topic'] == "temp-hum":
                                if (rev_data['temperature'] >= 0 and rev_data['temperature'] <= 1200):
                                    temp.append(rev_data['temperature'])
                                if (rev_data['humidity'] >= 0 and rev_data['humidity'] <= 100):
                                    hum.append(rev_data['humidity'])

                        if len(smoke) > 4 and len(temp) > 4 and len(hum) > 4:
                            smoke_data = temp_data = hum_data= 0
                            for i in range(5):
                                smoke_data += smoke.pop(0)
                                temp_data += temp.pop(0)
                                hum_data += hum.pop(0)
                            smoke.clear()
                            temp.clear()
                            hum.clear()
                            env_data = json.dumps({'smoke': smoke_data/5, 'temp': temp_data/5, 'hum': hum_data/5})
                            self.mqtt_client.publish("status/environments", env_data, 1)
                            print('Published data to server:', env_data)
                # Do something with the received message (e.g., publish to MQTT)
            
    # Define the MQTT event handlers
    def on_connect(self, client, userdata, flags, rc):
        print('MQTT Connected')
        self.mqtt_client.subscribe('controller/buzzer')  # Subscribe to a topic
        self.mqtt_client.publish("connected", "Rasp", 1)

    def on_message(self, client, userdata, msg):
        print('MQTT Message:', msg.payload.decode())
        if (msg.topic == "controller/buzzer"):
            print('MQTT Message:', msg.topic)
            for client_socket in self.sockets_list:
                if client_socket != self.server_socket:
                    data_buzzer = {"topic": "buzzer", "data": msg.payload.decode()}
                    data_buzzer_json = json.dumps(data_buzzer)
                    client_socket.sendall(data_buzzer_json.encode())
                    print(data_buzzer_json)
    def stop(self):
        for socket in self.sockets_list:
            socket.close()
        self.server_socket.close()

mqtt_client = mqtt.Client()

# Connect to the MQTT broker
mqtt_client.username_pw_set('eXUeRHMZWmaE7CqW9nj2Peio1iKtDrNFdyHS3jGNtGgq6wa6KjYpn5CdbcuCs87v', '')  # Set your MQTT username and password here

mqtt_client.connect("mqtt.flespi.io", 1883, 60)

# Start the Socket server
socket_server = SocketServer('0.0.0.0', 5000, mqtt_client)

mqtt_client.loop_start()

socket_server.start()

socket_server.run()


