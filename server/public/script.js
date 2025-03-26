const ws = new WebSocket("wss://esp8266-pager.onrender.com/");

const statusEl = document.getElementById("status");
const errorEl = document.getElementById("error-text");
const sendIcon = document.getElementById("send-icon");
const loadingSpinner = document.getElementById("loading-spinner");
const input = document.getElementById("input");
const button = document.getElementById("button");

const MESSAGE_RECEIVED_RESPONE = "--ESP8266: Message received--";
let responseTimeout;

ws.onopen = function () {
  statusEl.textContent = "Connected";
  statusEl.style.color = "green";
};

ws.onmessage = function (event) {
  console.log("Message from server:", event.data);

  clearTimeout(responseTimeout);
  errorEl.style.display = "none";

  if (event.data === MESSAGE_RECEIVED_RESPONE) {
    sendIcon.style.display = "block";
    loadingSpinner.style.display = "none";
    button.disabled = false;
  }
};

ws.onerror = function (error) {
  statusEl.textContent = "Error";
  status.El.style.color = "red";
};

ws.onclose = function (event) {
  statusEl.textContent = "Closed";
  status.El.style.color = "red";
};

function sendNewMessage(e) {
  if (input.value === "") return;

  sendIcon.style.display = "none";
  loadingSpinner.style.display = "block";
  button.disabled = true;

  ws.send(input.value);

  responseTimeout = setTimeout(() => {
    errorEl.style.display = "block";
    sendIcon.style.display = "block";
    loadingSpinner.style.display = "none";
    button.disabled = false;
  }, 5000);
}
