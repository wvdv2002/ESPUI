#include "ESPUI.h"

#include "uploadDataIndex.h"
#include "uploadDataControls.h"
#include "uploadDataStyle.h"

#include "uploadDataZepto.h"
#include "uploadDataNormalize.h"

#include <ESPAsyncWebServer.h>
#include <functional>

void ESPUIClass::prepareFileSystem(){
// this function should only be used once, maybe even halt the system
/*
Needed Things
index.htm HTML_INDEX

style/style.css
stye/normalize.css

js/controls.js
js/zepto.js

*/
if(!SPIFFS.begin()){
    if(debug) Serial.println("SPIFFS Mount Failed");
    return;
}

/*
listDir(SPIFFS, "/", 0);
writeFile(SPIFFS, "/hello.txt", "Hello ");
appendFile(SPIFFS, "/hello.txt", "World!\n");
readFile(SPIFFS, "/hello.txt");
deleteFile(SPIFFS, "/foo.txt");
renameFile(SPIFFS, "/hello.txt", "/foo.txt");
readFile(SPIFFS, "/foo.txt");
testFileIO(SPIFFS, "/test.txt");
*/

}


// Handle Websockets Communication
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_DISCONNECT:
    if (debug)
      Serial.printf("Disconnected!\n");
    break;
  case WS_EVT_CONNECT: {
    if (debug)
      Serial.println("Connected");
    ESPUI.jsonDom(client);
    if (debug)
      Serial.println("JSON Data Sent to Client!");
  } break;
  case WS_EVT_DATA:
    String msg = "";
    for (size_t i = 0; i < len; i++) {
      msg += (char)data[i];
    }
    
    Control *c = ESPUI.controls[msg.substring(msg.lastIndexOf(':') + 1).toInt()];

    if (msg.startsWith("bdown:")) {
      c->callback(*c, B_DOWN);
    } else if (msg.startsWith("bup:")) {
      c->callback(*c, B_UP);
    } else if (msg.startsWith("pfdown:")) {
      c->callback(*c, P_FOR_DOWN);
    } else if (msg.startsWith("pfup:")) {
      c->callback(*c, P_FOR_UP);
    } else if (msg.startsWith("pldown:")) {
      c->callback(*c, P_LEFT_DOWN);
    } else if (msg.startsWith("plup:")) {
      c->callback(*c, P_LEFT_UP);
    } else if (msg.startsWith("prdown:")) {
      c->callback(*c, P_RIGHT_DOWN);
    } else if (msg.startsWith("prup:")) {
      c->callback(*c, P_RIGHT_UP);
    } else if (msg.startsWith("pbdown:")) {
      c->callback(*c, P_BACK_DOWN);
    } else if (msg.startsWith("pbup:")) {
      c->callback(*c, P_BACK_UP);
    } else if (msg.startsWith("pcdown:")) {
      c->callback(*c, P_CENTER_DOWN);
    } else if (msg.startsWith("pcup:")) {
      c->callback(*c, P_CENTER_UP);
    } else if (msg.startsWith("sactive:")) {
      ESPUI.updateSwitcher(c->id, true);
      c->callback(*c, S_ACTIVE);
    } else if (msg.startsWith("sinactive:")) {
      ESPUI.updateSwitcher(c->id, false);
      c->callback(*c, S_INACTIVE);
    }
    break;
  }
}

void ESPUIClass::label(const char *label, int color, String value) {
  if (labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element " + String(label) +
                     " exists, skipping creating element!");
    return;
  }

  Control *newL = new Control();
  newL->type = UI_LABEL;
  newL->label = label;
  newL->color = color;
  if (value != "")
    newL->value = value; // Init with labeltext
  else
    newL->value = String(label);
  newL->callback = NULL;
  newL->id = cIndex;
  controls[cIndex] = newL;
  cIndex++;
}

// TODO: this still needs a range setting
void ESPUIClass::slider(const char *label, void (*callBack)(Control, int), int color, String value) {
  if (labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element " + String(label) +
                     " exists, skipping creating element!");
    return;
  }

  Control *newSL = new Control();
  newSL->type = UI_SLIDER;
  newSL->label = label;
  newSL->color = color;
  if (value != "")
    newSL->value = value;
  else
    newSL->value = ""; // TODO: init with half value
  newSL->callback = callBack;
  newSL->id = cIndex;
  controls[cIndex] = newSL;
  cIndex++;
}

void ESPUIClass::button(const char *label, void (*callBack)(Control, int),
                        int color, String value) {
  if (labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element " + String(label) +
                     " exists, skipping creating element!");
    return;
  }

  Control *newB = new Control();
  newB->type = UI_BUTTON;
  newB->label = label;
  newB->color = color;

  if (value != "")
    newB->value = value; // Init with labeltext
  else
    newB->value = String(label);

  newB->callback = callBack;
  newB->id = cIndex;
  controls[cIndex] = newB;
  cIndex++;
}

void ESPUIClass::switcher(const char *label, bool startState,
                          void (*callBack)(Control, int), int color) {
  if (labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element " + String(label) +
                     " exists, skipping creating element!");
    return;
  }

  Control *newS = new Control();
  newS->type = UI_SWITCHER;
  newS->label = label;
  newS->color = color;
  newS->value = String(startState);
  newS->callback = callBack;
  newS->id = cIndex;
  controls[cIndex] = newS;
  cIndex++;
}

void ESPUIClass::pad(const char *label, bool center,
                     void (*callBack)(Control, int), int color) {
  if (labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element " + String(label) +
                     " exists, skipping creating element!");
    return;
  }

  Control *newP = new Control();
  if (center)
    newP->type = UI_CPAD;
  else
    newP->type = UI_PAD;
  newP->label = label;
  newP->color = color;
  newP->callback = callBack;
  newP->id = cIndex;
  controls[cIndex] = newP;
  cIndex++;
}

void ESPUIClass::print(int id, String value) {
  if (id < cIndex && controls[id]->type == UI_LABEL) {
    controls[id]->value = value;
    String json;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["type"] = UPDATE_LABEL;
    root["value"] = value;
    root["id"] = String(id);
    root.printTo(json);
    this->ws->textAll(json);
  } else {
    if (debug)
      Serial.println(String("Error: ") + String(id) + String(" is no label"));
  }
}

void ESPUIClass::print(String label, String value) {
  if (!labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element does not " + String(label) +
                     " exist, cannot update!");
    return;
  }
  print(getIdByLabel(label), value);
}

void ESPUIClass::updateSwitcher(int id, bool nValue) {
  if (id < cIndex && controls[id]->type == UI_SWITCHER) {
    controls[id]->value = nValue ? 1 : 0;
    String json;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["type"] = UPDATE_SWITCH;
    root["value"] = nValue ? 1 : 0;
    root["id"] = String(id);
    root.printTo(json);
    this->ws->textAll(json);
  } else {
    if (debug)
      Serial.println(String("Error: ") + String(id) +
                     String(" is no switcher"));
  }
}

void ESPUIClass::updateSwitcher(String label, bool nValue) {
  if (!labelExists(label)) {
    if (debug)
      Serial.println("UI ERROR: Element does not " + String(label) +
                     " exist, cannot update!");
    return;
  }
  updateSwitcher(getIdByLabel(label), nValue);
}

int ESPUIClass::getIdByLabel(String label) {
  for (int i = 0; i < cIndex; i++) {
    if (String(controls[i]->label) == label)
      return i;
  }
  return -1; // failed, nonexistant
}

bool ESPUIClass::labelExists(String label) {
  for (int i = 0; i < cIndex; i++) {
    if (String(controls[i]->label) == label)
      return true;
  }
  return false;
}

// Convert & Transfer Arduino elements to JSON elements
void ESPUIClass::jsonDom(AsyncWebSocketClient *client) {
  for (int i = -1; i < cIndex; i++) {
    String json;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    if (i == -1) {
      root["type"] = UI_TITEL;
      root["label"] = String(ui_title);
    } else {
      root["type"] = controls[i]->type;
      root["label"] = String(controls[i]->label);
      root["value"] = String(controls[i]->value);
      root["color"] = String(controls[i]->color);
      root["id"] = String(i);
    }
    root.printTo(json);
    client->text(json);
  }
}

void ESPUIClass::begin(const char *_title) {

  ui_title = _title;
  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/ws");
  SPIFFS.begin();
  ws->onEvent(onWsEvent);
  server->addHandler(ws);
  server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  // Heap for general Servertest
  server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server->onNotFound(
      [](AsyncWebServerRequest *request) { request->send(404); });

  server->begin();
  if (debug)
    Serial.println("UI Initialized");
}

ESPUIClass ESPUI;
