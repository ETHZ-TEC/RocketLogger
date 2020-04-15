# RocketLogger Node.js Web Interface

Interface features include:

- static source file serving
- dynamic site redering using nunjucks
- RocketLogger CLI interface from web
- websockets for server side data streaming using socket.io and zeromq


## Node.js server side dependencies

- `express` - minimalistic web framework
- `nunjucks` - Jinja2 inspired web template engine
- `socket.io` - websockets server side library
- `zeromq` - message queueing library
- `gulp` - file system listing

Install dependencies using `npm`:

`npm install express nunjucks gulp socket.io zeromq`


## Browser client side dependencies

- `bootstrap` - responsive, mobile-first front-end component library
- `popper.js` - tooltip and popover positioning engine (used by bootstrap)
- `jquery` - general purpose JavaScript library
- `socket.io.js` - websockets client side JavaScript library
- `flot` - JavaScript plotting library
