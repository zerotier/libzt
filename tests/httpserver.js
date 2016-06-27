var http = require('http');

var server = http.createServer(function (request, response) {
  response.writeHead(200, {"Content-Type": "text/plain"});
  response.end("Welcome to the machine!\n");
});

server.listen(8777);

console.log("Server running at http://127.0.0.1:8777/");
