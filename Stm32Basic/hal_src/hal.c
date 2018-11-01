#include "hal.h"

/* GLobal variables */
static volatile uint32_t systick_cnt;

void clock_setup(void)
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();    /* 72 MHz */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_AFIO);
    systick_setup();
}

void usart_setup(void)
{
    /* Setup GPIO pin GPIO_USART1_TX/GPIO9 on GPIO port A for transmit. */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

    /* Setup UART parameters. */
    usart_set_baudrate(USART1, UART_SPEED);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    /* Finally enable the USART. */
    usart_enable(USART1);
}

void usart_send_string(uint32_t usart, const char *string, uint16_t str_size)
{
    uint16_t iter = 0;
    do
    {
        usart_send_blocking(usart, string[iter++]);
    }while(string[iter] != 0 && iter < str_size);
}

void uart_write_number(uint32_t usart, uint32_t num)
{
    char value[16];
    uint8_t i = 0;

    do
    {
        value[i++] = (char)(num % 10) + '0';
        num /= 10;
    } while(num);

    while(i)
    {
        usart_send_blocking(usart, value[--i]);
    }
}

void init_KBD(void)
{
    /* Set KBD input pins */
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_FLOAT,
        KB1_1 | KB1_2 | KB1_3 | KB1_4 | KB1_5);

    /* Set KBD output pins */
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        KB2_1 | KB2_2 | KB2_3 | KB2_4 | KB2_5 | KB2_6 | KB2_7 | KB2_8);
}

void reset_KBD2(void)
{
    gpio_set(
        GPIOB,
        KB2_1 | KB2_2 | KB2_3 | KB2_4 | KB2_5 | KB2_6 | KB2_7 | KB2_8);
}

void set_KBD2(int pin)
{
    reset_KBD2();

    switch(pin)
    {
        case 6:
            gpio_clear(GPIOB, KB2_6);
            break;

        case 4:
            gpio_clear(GPIOB, KB2_4);
            break;

        case 2:
            gpio_clear(GPIOB, KB2_2);
            break;

        case 1:
            gpio_clear(GPIOB, KB2_1);
            break;

        case 3:
            gpio_clear(GPIOB, KB2_3);
            break;

        case 5:
            gpio_clear(GPIOB, KB2_5);
            break;

        case 7:
            gpio_clear(GPIOB, KB2_7);
            break;

        case 8:
            gpio_clear(GPIOB, KB2_8);
            break;

        default:
            break;
    }
}

uint16_t read_KBD1(void)
{
    uint16_t ret = gpio_get(GPIOA, KB1_1);
    ret |= gpio_get(GPIOA, KB1_2);
    ret |= gpio_get(GPIOA, KB1_3);
    ret |= gpio_get(GPIOA, KB1_4);
    ret |= gpio_get(GPIOA, KB1_5) >> 4;

    ret = 0x1F & (~ret);
    return ret;
}

void i2c_setup(void)
{
    /* Set alternate functions for the SCL and SDA pins of I2C2. */
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
        LCD_I2C_SCL | LCD_I2C_SDA);

    /* Disable the I2C before changing any configuration. */
    i2c_peripheral_disable(I2C1);

    /* APB1 is running at 36MHz. */
    i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);

    /* 400KHz - I2C Fast Mode */
    i2c_set_fast_mode(I2C1);

    /*
     * fclock for I2C is 36MHz APB2 -> cycle time 28ns, low time at 400kHz
     * incl trise -> Thigh = 1600ns; CCR = tlow/tcycle = 0x1C,9;
     * Datasheet suggests 0x1e.
     */
    i2c_set_ccr(I2C1, 0x1e);

    /*
     * fclock for I2C is 36MHz -> cycle time 28ns, rise time for
     * 400kHz => 300ns and 100kHz => 1000ns; 300ns/28ns = 10;
     * Incremented by 1 -> 11.
     */
    i2c_set_trise(I2C1, 0x0b);

    /*
     * This is our slave address - needed only if we want to receive from
     * other masters.
     */
    /* i2c_set_own_7bit_slave_address(I2C2, 0x32); */

    /* If everything is configured -> enable the peripheral. */
    i2c_peripheral_enable(I2C1);
}

void i2c_write_byte(uint8_t address, uint8_t data)
{
    /* Send START condition. */
    i2c_send_start(I2C1);

    /* Waiting for START is send and switched to master mode. */
    while (!((I2C_SR1(I2C1) & I2C_SR1_SB)
    & (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

    /* Send destination address. */
    i2c_send_7bit_address(I2C1, address, I2C_WRITE);

    /* Waiting for address is transferred. */
    while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR));

    /* Cleaning ADDR condition sequence. */
    I2C_SR2(I2C1);

    /* Sending the data. */
    i2c_send_data(I2C1, data);
    while (!(I2C_SR1(I2C1) & I2C_SR1_BTF));

    /* After the last byte we have to wait for TxE too. */
    while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF | I2C_SR1_TxE)));

    /* Send STOP condition. */
    i2c_send_stop(I2C1);
}

void systick_setup(void)
{
    systick_cnt = 0;

    /* 72MHz / 8 => 9000000 counts per second */
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

    /* 9000000/900 = 10000 overflows per second - every 0.1 ms one interrupt */
    /* SysTick interrupt every N clock pulses: set reload to N-1 */
    systick_set_reload(899);
    systick_interrupt_enable();

    /* Start counting. */
    systick_counter_enable();
}

void sys_tick_handler(void)
{
    systick_cnt++;
}

void delay_us100(uint32_t us100)
{
    systick_cnt = 0;
    while(systick_cnt < us100);
}
