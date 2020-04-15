# RocketLogger Node.js Web Interface

Interface features include:

- static source file serving
- dynamic site redering using nunjucks
- RocketLogger CLI interface from web
- websockets for server side data streaming using socket.io and zeromq


## nodejs packet requirements

- `express` - minimalistic web framework
- `nunjucks` - Jinja2 inspired web template engine
- `socket.io` - websockets library
- `zeromq` - message queueing library
- `gulp` - file system listing

Install dependencies using `npm`:

`npm install express nunjucks gulp socket.io zeromq`
