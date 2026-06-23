#!/bin/bash

set -e

APP_DIR="$HOME/Documents/BeanChatServer"
BUILD_DIR="$APP_DIR/build/Desktop_Qt_6_9_3-Release"
DEPLOY_DIR="$APP_DIR/deployed"

echo "Cleaning old deployment..."
rm -rf "$DEPLOY_DIR"

echo "Installing application..."
cmake --install "$BUILD_DIR" --prefix "$DEPLOY_DIR"

echo "Creating sqldrivers directory..."
mkdir -p "$DEPLOY_DIR/bin/sqldrivers"

echo "Copying SQLite plugin..."
cp "$HOME/Qt/6.9.3/gcc_64/plugins/sqldrivers/libqsqlite.so" \
   "$DEPLOY_DIR/bin/sqldrivers/"

echo "Deployment complete."

echo
echo "Result:"
#tree -L 2 "$DEPLOY_DIR"
