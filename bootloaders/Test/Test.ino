void setup() {
  Serial.begin(115200);        // 初始化串口，波特率 9600
  while (!Serial) { }        // 对于 Leonardo / Micro 有用，其它板子无影响
  Serial.println("Hello Arduino!");
}

void loop() {
  Serial.println("Loop running...");
  delay(1000);               // 每 1 秒打印一次
}
