# ProxyLab

<p>this is a course lab for CS:APP(2013 Spring) of Fudan University</p>

This directory contains the files you will need for the CS:APP Proxy Lab.
<pre>The file directory contains:
proxy.c
  the main file where we implemented the proxy logic
csapp.h
  the header file of csapp.c
csapp.c
  the library which provides the Rio and other helper functions
sbuf.h
  the header file of sbuf.c
sbuf.c
  the file which implements the thread-safe buffer
Makefile
  This is the makefile to build the proxy program.  Type "make all" in your terminal to build</pre>
  
The tiny proxy server is based on prethreading. When the proxy start, it creates N worker threads which consume the items in the sbuf. The main thread listens to the port 8080 and when user connects, it inserts item(client fd, client socket address) into the sbuf. To protect the sbuf, I use Semaphore.

This tiny server now can hanle GET method. You could test it through cURL.
For example: 
<pre><code>unix>curl www.example.com -x http://localhost:8080</code></pre>
