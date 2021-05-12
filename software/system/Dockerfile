# RocketLogger image patching environment
FROM arm32v7/debian:buster-slim

# Debian package dependencies
RUN apt-get update && apt-get install --assume-yes  \
    git

# default shell config
WORKDIR /home
CMD ["bash"]
