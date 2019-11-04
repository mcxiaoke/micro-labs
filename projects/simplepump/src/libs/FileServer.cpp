#include "FileServer.h"

bool FileServer::handle(SERVER_CLASS* server) {
  String path = server->uri();
  LOGN("[FileServer] Handling path: " + path);
  if (path.endsWith("/"))
    path += "index.html";
  String contentType;
  if (path.endsWith(".htm") || path.endsWith(".html"))
    contentType = "text/html";
  else if (path.endsWith(".css"))
    contentType = "text/css";
  else if (path.endsWith(".js"))
    contentType = "application/javascript";
  else if (path.endsWith(".png"))
    contentType = "image/png";
  else if (path.endsWith(".gif"))
    contentType = "image/gif";
  else if (path.endsWith(".jpg"))
    contentType = "image/jpeg";
  else if (path.endsWith(".ico"))
    contentType = "image/x-icon";
  else if (path.endsWith(".xml"))
    contentType = "text/xml";
  else if (path.endsWith(".pdf"))
    contentType = "application/x-pdf";
  else if (path.endsWith(".zip"))
    contentType = "application/x-zip";
  else if (path.endsWith(".gz"))
    contentType = "application/x-gzip";
  else if (path.endsWith(".json"))
    contentType = "application/json";
  else
    contentType = "text/plain";

  // split filepath and extension
  String prefix = path, ext = "";
  int lastPeriod = path.lastIndexOf('.');
  if (lastPeriod >= 0) {
    prefix = path.substring(0, lastPeriod);
    ext = path.substring(lastPeriod);
  }

  // look for smaller versions of file
  // minified file, good (myscript.min.js)
  if (SPIFFS.exists(prefix + ".min" + ext))
    path = prefix + ".min" + ext;
  // gzipped file, better (myscript.js.gz)
  if (SPIFFS.exists(prefix + ext + ".gz"))
    path = prefix + ext + ".gz";
  // min and gzipped file, best (myscript.min.js.gz)
  if (SPIFFS.exists(prefix + ".min" + ext + ".gz"))
    path = prefix + ".min" + ext + ".gz";

  if (SPIFFS.exists(path)) {
    LOGN("[FileServer] Sending file:" + path);
    File file = SPIFFS.open(path, "r");
    if (server->hasArg("download"))
      server->sendHeader("Content-Disposition", " attachment;");
    if (server->uri().indexOf("nocache") < 0)
      server->sendHeader("Cache-Control", " max-age=172800");

    // optional alt arg (encoded url), server sends redirect to file on the web
    if (server->hasArg("alt")) {
      server->sendHeader("Location", server->arg("alt"), true);
      server->send(302, "text/plain", "");
    } else {
      // server sends file
      server->streamFile(file, contentType);
    }
    file.close();
    return true;
  }  // if SPIFFS.exists
  return false;
}