#include <crow/app.h>

int main() {
  crow::SimpleApp app;

  CROW_ROUTE(app, "/")([] { return "Hello world"; });

  app.port(18080).multithreaded().run();
}
