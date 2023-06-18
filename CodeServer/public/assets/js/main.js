$.when(
  $.getScript('./configClient/config.js',function(){
      Socket_hostIP = hostIP;
      Socket_port = port;
  })).done(function(){
  var temperatureData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,] ; 
  var humidityData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,]; 
  var smokeData = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,]; 
  const temperatureCanvas = document.getElementById('temperatureChart');
  const humidityCanvas = document.getElementById('humidityChart');
  const smokeCanvas = document.getElementById('smokeChart');

  const tempValue = document.getElementById('currentTemperature');
  const humValue = document.getElementById('currentHumidity');
  const smokeValue = document.getElementById('currentSmoke');

  const statusButton = document.getElementById('status-btn');

  const ctx1 = temperatureCanvas.getContext('2d');
  const ctx2 = humidityCanvas.getContext('2d');
  const ctx3 = smokeCanvas.getContext('2d');

  const connectionObject = {
    withCredentials: true,
  };
  const socket = io("", connectionObject);
  // socket = io.connect('http://' + Socket_hostIP + ':' + Socket_port, { transports : ['websocket'] });
  socket.emit('Get-data');
  socket.on('Environment-update', ()=>{socket.emit('Get-data');})
  socket.on('Send-data', (data)=>{
    $("#status-btn").removeClass("btn-primary").addClass("btn-warning");
    statusButton.textContent = "Warning";
    // console.info(data)
    tempValue.textContent = data[0].temp + "°C";
    humValue.textContent = data[0].hum + "%";
    smokeValue.textContent = data[0].smoke + "mA";
    data.forEach((env, index) => {
      temperatureData[index] = env.temp;
      humidityData[index] = env.hum;
      smokeData[index] = env.smoke;
    });
    temperatureChart.data.datasets[0].data = temperatureData;
    temperatureChart.update();

    humidityChart.data.datasets[0].data = humidityData;
    humidityChart.update();

    smokeChart.data.datasets[0].data = smokeData;
    smokeChart.update();
  })
  statusButton.addEventListener('click', 
  ()=>{
        socket.emit('Buzzer');
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

  const smokeChart = new Chart(ctx3, {
    type: 'line',
    data: {
      labels: Array.from({ length: smokeData.length }, (_, i) => i + 1),
      datasets: [{
        label: 'Smoke',
        data: smokeData,
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
          text: 'Smoke (mA)'
        }
      }
    }
  });
})
