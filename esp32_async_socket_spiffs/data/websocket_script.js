const socket = new WebSocket('ws://home:81/');
const ctx1 = document.getElementById('myChart1');
const ctx2 = document.getElementById('myChart2');

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

const sensorChart1 = new Chart(ctx1, {
  type: 'line',
  data: {
    labels: [
      '00:00:01','00:00:02','00:00:03','00:00:04','00:00:05','00:00:06','00:00:07',
      '00:00:08','00:00:09','00:00:10','00:00:11','00:00:12','00:00:13','00:00:14','00:00:15',
    ],
    datasets: [{
      label: '*C',
      data: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      borderWidth: 1
    }]
  },
  options: {
    scales: {
      y: {
        min: 0,
        max: 50
      }
    },
    animation: false,
    parse: false,
    normalized: true
  }
});

const sensorChart2 = new Chart(ctx2, {
  type: 'line',
  data: {
    labels: [
      '00:00:01','00:00:02','00:00:03','00:00:04','00:00:05','00:00:06','00:00:07',
      '00:00:08','00:00:09','00:00:10','00:00:11','00:00:12','00:00:13','00:00:14','00:00:15',
    ],
    datasets: [{
      label: '%RH',
      data: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      borderWidth: 1
    }]
  },
  options: {
    scales: {
      y: {
        min: 0,
        max: 100
      }
    },
    animation: false,
    parse: false,
    normalized: true
  }
});

function shiftChartData(chart, label, newData) {
  chart.data.labels.shift();
  chart.data.labels.push(label);
  chart.data.datasets.forEach((dataset) => {
      dataset.data.shift();
      dataset.data.push(newData);
  });
  chart.update('none');
}

function handle_dht(data) {
  document.getElementById('sensorData1').textContent = data.dht[0];
  document.getElementById('sensorData2').textContent = data.dht[1];
  document.getElementById('touchProgress1').style.width = "" + (data.dht[0] / 50  * 100) + "%";
  document.getElementById('touchProgress2').style.width = "" + (data.dht[1] / 100  * 100) + "%";

  const d = new Date();
  let time = "" + d.getHours() +":"+ d.getMinutes() +":"+ d.getSeconds();
  shiftChartData(sensorChart1, time, data.dht[0]);
  shiftChartData(sensorChart2, time, data.dht[1]);
}

function handle_light(data) {
  room = data.light;
  lights_element = document.getElementById(room).firstElementChild.lastElementChild;
  btn_classes = document.getElementById(room).firstElementChild.classList;

  if(lights_element.textContent == "Lights: ON") {
    lights_element.textContent = "Lights: OFF";
    btn_classes.replace("btn-warning", "btn-dark");
  }
  else {
    lights_element.textContent = "Lights: ON";
    btn_classes.replace("btn-dark", "btn-warning");
  }
}

function handle_pir() {
  btn_classes = document.getElementById('Backyard').firstElementChild.classList;
  body_classes = document.getElementById('html_body').classList;
  text = document.getElementById('Backyard').firstElementChild.lastElementChild;

  btn_classes.replace("btn-dark", "btn-danger");
  body_classes.replace("bg-black", "bg-danger");
  text.textContent = "Movement Detected";
  
  sleep(5000).then(() => {
    btn_classes.replace("btn-danger", "btn-dark");
    body_classes.replace("bg-danger", "bg-black");
    text.textContent = "";
  });
}

function handle_door(data) {
  btn_classes = document.getElementById('Door').firstElementChild.classList;
  door_text = document.getElementById('Door').firstElementChild.lastElementChild;

  if(data.door) {
    door_text.textContent = "Open";
    btn_classes.replace("btn-dark", "btn-warning");
  }
  else {
    door_text.textContent = "Close";
    btn_classes.replace("btn-warning", "btn-dark");
  }
}

socket.onopen = function() {
  console.log('WebSocket connection opened');
};

socket.onmessage = function(event) {
  console.log(event.data);

  const data = JSON.parse(event.data);
  for (const key in data) {
    if(key=='dht')
      handle_dht(data);
    if(key=='light')
      handle_light(data);
    if(key=='pir')
      handle_pir();
    if(key=='door')
      handle_door(data);
  }
};

socket.onclose = function() {
  console.log('WebSocket connection closed');
};

function lightClick(room) {
  socket.send(room);
}

function doorClick() {
  socket.send('door');
}