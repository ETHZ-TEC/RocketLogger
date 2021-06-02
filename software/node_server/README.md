# RocketLogger Node.js Web Interface

## Features

The RocketLogger web interface features are summarized as follows:

* static source file serving
* dynamic site rendering using nunjucks
* RocketLogger CLI interface from web
* websockets for server side data streaming using socket.io and zeromq


## Requirements

* Server: Node.js v10 or later, NGINX for reverse proxy
* Client: reasonably recent web browser supporting ECMAScript 6


### Node.js server dependencies

* `express` - minimalist web framework
* `nunjucks` - Jinja2 inspired web template engine
* `socket.io` - websockets server side library
* `zeromq` - ZeroMQ message queueing library
* `gulp` - file system listing


### Browser client dependencies

* `bootstrap` - responsive, mobile-first front-end component library
* `jquery` - general purpose JavaScript library
* `plotly.js` - JavaScript plotting library
* `timesync` - client-server time synchronization library
* `socket.io-client` - websockets client side JavaScript library


## Installation

To install the webserver, its dependencies, and deploy it as system service, use the provided installer script:
```bash
sudo ./install.sh
```

To install the node.js server standalone in the source directory use:
```
npm install .
```
