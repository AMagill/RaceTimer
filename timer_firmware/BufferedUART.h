#ifndef BUFFEREDUART_H_
#define BUFFEREDUART_H_

void uartInit();
void uartIntHandler();
void uartSend(const char *buffer, uint32_t count);
void uartPrint(const char *format, ...);

#endif /* BUFFEREDUART_H_ */
