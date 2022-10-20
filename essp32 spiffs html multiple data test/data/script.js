
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var LED1_STATE = 0;
var LED2_STATE = 0;
    
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
  LED1_STATE = data.LED1;
  LED2_STATE = data.LED2;
  var Temperature = data.TEMPERATURE;
  var Humidity = data.HUMIDITY;
  
  document.getElementById('state').innerHTML = LED1_STATE;
  document.getElementById('state2').innerHTML = LED2_STATE;
    
  document.getElementById('temp_meter').value = Temperature;
  document.getElementById('temp_val').innerHTML = Temperature;
    
  document.getElementById('hum_meter').value = Humidity;
  document.getElementById('hum_val').innerHTML = Humidity;
}
function onLoad(event) {
  initWebSocket();
  initButton();
}
function initButton() {
  document.getElementById('button').addEventListener('click', toggle1);
  document.getElementById('button2').addEventListener('click', toggle2);
}
function toggle1(){
  if(LED1_STATE == 0){
    LED1_STATE = 1;
  }else{
    LED1_STATE = 0;
  }
  console.log("Toggle LED1 State to: "+LED1_STATE);
  sendFullData();
}
function toggle2(){
  if(LED2_STATE == 0){
    LED2_STATE = 1;
  }else{
    LED2_STATE = 0;
  }
  console.log("Toggle LED2 State to: "+LED2_STATE);
  sendFullData();
}
function sendFullData(){
  var fullData = '{"LED1" :'+LED1_STATE+', "LED2" :'+LED2_STATE+'}';
  console.log(fullData);
  websocket.send(fullData);
}