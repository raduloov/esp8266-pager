const express = require("express");
const app = express();
const server = require("http").createServer(app);
const WebSocket = require("ws");

const wss = new WebSocket.Server({ server });

wss.on("connection", function connection(ws) {
  console.log("A new client connected!");
  ws.send("Welcome client!");

  ws.on("message", function incoming(message, isBinary) {
    console.log("received: %s", message);

    wss.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(message, { binary: isBinary });
      }
    });
  });
});

app.get("/", (req, res) => res.json({ message: "Hello World!" }));
console.log("Server started!");

server.listen(3000, () => console.log(`Listening on port ${3000}`));
