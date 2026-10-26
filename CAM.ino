#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

const char* WIFI_SSID = "Kalam_004";
const char* WIFI_PASSWORD = "kalam@2026";

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t index_handler(httpd_req_t *req) {
  String ip = WiFi.localIP().toString();
  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PipeGuard Camera</title><style>body{font-family:Arial;background:#f3f6fb;margin:0;padding:18px;color:#17263b}"
    ".card{max-width:900px;margin:auto;background:white;padding:16px;border-radius:14px;border:1px solid #dce4ee}"
    "img{width:100%;background:#111;border-radius:8px}</style></head><body><div class='card'>"
    "<h2>PipeGuard M21-45 Camera</h2><p>Camera IP: " + ip + "</p><img src='http://" + ip +
    ":81/stream'><p>Stream URL: http://" + ip + ":81/stream</p></div></body></html>";
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, html.c_str(), HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");

  char part_buf[64];

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[CAM] Frame capture failed");
      return ESP_FAIL;
    }

    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;
    bool converted = false;

    if (fb->format == PIXFORMAT_JPEG) {
      jpg_buf = fb->buf;
      jpg_len = fb->len;
    } else {
      converted = frame2jpg(fb, 65, &jpg_buf, &jpg_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!converted) {
        Serial.println("[CAM] RGB565-to-JPEG conversion failed");
        if (jpg_buf) free(jpg_buf);
        return ESP_FAIL;
      }
    }

    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) {
      size_t header_len = snprintf(part_buf, sizeof(part_buf), STREAM_PART, (unsigned int)jpg_len);
      res = httpd_resp_send_chunk(req, part_buf, header_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char*)jpg_buf, jpg_len);
    }

    if (fb) esp_camera_fb_return(fb);
    else if (jpg_buf) free(jpg_buf);

    if (res != ESP_OK) break;
    vTaskDelay(1);
  }
  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;

  httpd_uri_t index_uri = {};
  index_uri.uri = "/";
  index_uri.method = HTTP_GET;
  index_uri.handler = index_handler;
  index_uri.user_ctx = NULL;

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  } else {
    Serial.println("[WEB] Camera page server failed");
  }

  config.server_port = 81;
  config.ctrl_port = 32769;

  httpd_uri_t stream_uri = {};
  stream_uri.uri = "/stream";
  stream_uri.method = HTTP_GET;
  stream_uri.handler = stream_handler;
  stream_uri.user_ctx = NULL;

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  } else {
    Serial.println("[WEB] Stream server failed");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(800);
  Serial.println();
  Serial.println("PipeGuard M21-45 ESP32-CAM starting...");

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (!psramFound()) {
    Serial.println("[WARN] PSRAM not detected. Raw RGB565 QVGA may not have enough memory.");
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAM] Camera init failed: 0x%x\n", err);
    Serial.println("[CAM] Check ribbon orientation/latch, sensor compatibility and ESP32 camera driver version.");
    while (true) delay(1000);
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor) {
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  }
  Serial.println("[CAM] Camera initialized in RGB565 QVGA mode.");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WIFI] Connecting to mobile hotspot: ");
  Serial.println(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Connection failed. Check hotspot SSID/password and 2.4 GHz mode.");
    while (true) delay(1000);
  }

  Serial.println("[WIFI] Connected to mobile hotspot.");
  Serial.print("[WIFI] Camera IP: http://");
  Serial.println(WiFi.localIP());
  Serial.println("[WEB] Starting camera page and stream...");
  startCameraServer();
  Serial.print("[CAM] Test page: http://");
  Serial.println(WiFi.localIP());
  Serial.print("[CAM] Stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Disconnected; retrying hotspot connection...");
    WiFi.reconnect();
    delay(1000);
  }
  delay(100);
}
