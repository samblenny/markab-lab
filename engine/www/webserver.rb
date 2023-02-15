#!/usr/bin/ruby
# Copyright (c) 2022 Sam Blenny
# SPDX-License-Identifier: MIT
#
require 'webrick'

# Serve the current working directory on http://localhost:8000
# Turn off all logging
# Use correct mime type for .wasm files (ruby only added wasm type in v2.7)
config = {DocumentRoot: Dir.pwd,
          MimeTypes: WEBrick::HTTPUtils::DefaultMimeTypes.merge({"wasm" => "application/wasm"}),
          Logger: WEBrick::Log.new(open(File::NULL, 'w')),
          AccessLog: [],
          DoNotReverseLookup: true,
          Port: 8000}
server = WEBrick::HTTPServer.new(config)
trap 'INT' do server.shutdown end
server.mount('/', WEBrick::HTTPServlet::FileHandler, config[:DocumentRoot], {})
puts "listening on localhost:8000"
server.start
