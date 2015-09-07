# ProxyLab

<p>This is the web proxy lab of CS:APP, 13 Spring, ECE, Fudan University</p>
<p>The main files are</p>
<pre>
proxy.c
  the main proxy logic is here
csapp.c
  the Rio library and other helper functions
sbuf.c
  the thread-safe buffer
cache.c
  the thread-safe cache list
</pre>
<p>You can test it through cURL:</p>
<pre><code>unix>./proxy</code>
<code>unix>curl http://www.example.com -v -x http://localhost:8080</code></pre>
