const express = require("express");
const app = express();
const path = require("path");
const server = require("http").createServer(app);
const WebSocket = require("ws");

// serve the ui
app.use("/public", express.static(path.join(__dirname, "public")));
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "public", "index.html"));
});

const wss = new WebSocket.Server({ server });

wss.on("connection", function connection(ws) {
  console.log("A new client connected!");

  // inform ESP8266 that the WebSocket connection has been established
  ws.send("--WEBSOCKET SERVER: CONNECTED--");

  ws.on("message", function incoming(message, isBinary) {
    console.log("received: %s", message);

    wss.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(message, { binary: isBinary });
      }
    });
  });
});

const port = process.env.PORT || 3000;

server.listen(port, () => console.log(`Listening on port ${port}`));
