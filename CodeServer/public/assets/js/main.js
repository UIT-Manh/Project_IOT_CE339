$.when(
  $.getScript('./configClient/config.js',function(){
      Socket_hostIP = hostIP;
      Socket_port = port;
  })).done(function(){
  var temperatureData = [25, 26, 27, 28, 29, 25, 26, 50, 28, 29, 25, 26, 27, 28, 29, 25, 26, 27, 28, 29, 25, 26, 27, 28, 29, 25, 26, 27, 28, 29 ]; 
  var humidityData = [60, 62, 64, 65, 63, 60, 62, 64, 65, 63, 60, 62, 64, 65, 63, 60, 62, 64, 65, 63, 60, 62, 64, 65, 63, 60, 62, 64, 65, 63 ]; 
  
  const temperatureCanvas = document.getElementById('temperatureChart');
  const humidityCanvas = document.getElementById('humidityChart');
  const ctx1 = temperatureCanvas.getContext('2d');
  const ctx2 = humidityCanvas.getContext('2d');
  socket = io.connect('http://' + Socket_hostIP + ':' + Socket_port, { transports : ['websocket'] });
  socket.emit('Get-data');
  socket.on('Environment-update', ()=>{socket.emit('Get-data');})
  socket.on('Send-data', (data)=>{
    console.info(data)
    data.forEach((env, index) => {
      temperatureData[index] = env.temp;
      humidityData[index] = env.hum;
    });
    temperatureChart.data.datasets[0].data = temperatureData;
    temperatureChart.update();

    humidityChart.data.datasets[0].data = humidityData;
    humidityChart.update();
  })
  const temperatureChart = new Chart(ctx1, {
    type: 'line',
    data: {
      labels: Array.from({ length: temperatureData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Temperature',
        data: temperatureData,
        fill: false,
        borderColor: '#ff6384',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false
        }
      },
      plugins: {
        legend: {
          display: false
        },
        title: {
          display: true,
          text: 'Temprature (°C)'
        }
      }
      
    }
  });

  const humidityChart = new Chart(ctx2, {
    type: 'line',
    data: {
      labels: Array.from({ length: humidityData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Humidity',
        data: humidityData,
        fill: false,
        borderColor: '#36a2eb',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false
        }
      },
      plugins: {
        legend: {
          display: false
        },
        title: {
          display: true,
          text: 'Humidity (%)'
        }
      }
    }
  });
})
