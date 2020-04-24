# RocketLogger Node.js Web Interface


## Features

This web interface features:

- static source file serving
- dynamic site redering using nunjucks
- RocketLogger CLI interface from web
- websockets for server side data streaming using socket.io and zeromq


## Requirements

- Server: Node.js v10 or later
- Client: reasonably recent web browser supporting ECMAScript 6


### Node.js server side dependencies

- `express` - minimalistic web framework
- `nunjucks` - Jinja2 inspired web template engine
- `socket.io` - websockets server side library
- `zeromq` - message queueing library
- `gulp` - file system listing

Install dependencies using `npm`:

`npm install express nunjucks gulp socket.io zeromq`


### Browser client side dependencies

- `bootstrap` - responsive, mobile-first front-end component library
- `popper.js` - tooltip and popover positioning engine (used by bootstrap)
- `jquery` - general purpose JavaScript library
- `socket.io.js` - websockets client side JavaScript library
- `plotly.js` - JavaScript plotting library

Install dependencies using `npm`:

`npm install bootstrap jquery @popperjs/core socket.io-client plotly.js`


## Installation

TBD, provide install script
