# FreeRTOS DMA_Uart Freemodbus STM32_RTO and CRC_hardware_DMA

This is a modbus STM32 project using Freemodbus. FreeRTOS manage varous of different tasks. In order to achieve the maximum capibility of STM32 hardware, DMA was used for diferent peripherals, such as uart, crc. As RTO (receiver timeout) in uart was used, a modbus frame can be obtain through uart RTO interrupt. CRC is used as well, and DMA will transfer data from uart receive memory to CRC data register, which can significantly reduce the CPU utilization.   

## UART DMA
Uart DMA 
