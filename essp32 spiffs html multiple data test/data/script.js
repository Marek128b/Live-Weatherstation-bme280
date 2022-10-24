
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
    
window.addEventListener('load', onLoad);
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}
function onOpen(event) {
  console.log('Connection opened');
}
function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, -1);
}
function onMessage(event) {
  var fullData = event.data;
  console.log(fullData);
    
  var data = JSON.parse(fullData);
  var Temperature = data.TEMPERATURE;
  var Humidity = data.HUMIDITY;
  var Pressure = data.PRESSURE;
  
  document.getElementById('temp_meter').value = Temperature;
  document.getElementById('temp_val').innerHTML = Temperature;
    
  document.getElementById('hum_meter').value = Humidity;
  document.getElementById('hum_val').innerHTML = Humidity;

  document.getElementById('pres_meter').value = Pressure;
  document.getElementById('pres_val').innerHTML = Pressure;
}
function onLoad(event) {
  initWebSocket();
  initButton();
}