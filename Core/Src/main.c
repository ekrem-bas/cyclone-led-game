/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h> // abs() fonksiyonu için
#include <stdbool.h> // bool veri tipi için
#include <stdint.h>  // uint8_t, uint16_t gibi tipler için
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_tim3_ch1_trig;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define noOfLEDs 40
uint16_t pwmData[24 * noOfLEDs + 50]; // Bu dizi main.c'de tanımlı olmalı

// -----------------------------------------------------------
// WS2812 TEST FONSKIYONLARI
// -----------------------------------------------------------
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
	htim3.Instance->CCR1 = 0;
}

// tum ledleri off yapma fonksiyonu ?????
void resetAllLED(void) {
	for (int i = 0; i < 24 * noOfLEDs; i++)
		pwmData[i] = 1;
}

// tum ledleri beyaz renkte yakiyor sanirim ????
void setAllLED(void) {
	// set icin tum ledlerin datasini 2 yapıyoz ?????
	for (int i = 0; i < 24 * noOfLEDs; i++)
		pwmData[i] = 2;
}

// led'in rengini ayarlamak icin fonksiyon
void setLED(int LEDposition, int Red, int Green, int Blue) {
	for (int i = 7; i >= 0; i--) // Set the first 8 out of 24 to green
			{
		pwmData[24 * LEDposition + 7 - i] = ((Green >> i) & 1) + 1;
	}
	for (int i = 7; i >= 0; i--) // Set the second 8 out of 24 to red
			{
		pwmData[24 * LEDposition + 15 - i] = ((Red >> i) & 1) + 1;
	}
	for (int i = 7; i >= 0; i--) // Set the third 8 out of 24 to blue
			{
		pwmData[24 * LEDposition + 23 - i] = ((Blue >> i) & 1) + 1;
	}
}

// DMA baslatan fonksiyon
void ws2812Send(void) {
	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*) pwmData,
			24 * noOfLEDs + 50);
}
// -----------------------------------------------------------
// WS2812 TEST FONKSIYONLARI SON
// -----------------------------------------------------------

// -----------------------------------------------------------
// GEMINI OYUN KODU
// -----------------------------------------------------------
// Arduino kodundan gelen tanımlar

volatile bool Playing = true;      // volatile olarak işaretlendi
volatile bool CycleEnded = true;   // volatile olarak işaretlendi
uint32_t last_button_interrupt_time = 0; // Debounce için
#define DEBOUNCE_TIME_MS 100         // 50ms debounce süresi (ayarlayabilirsiniz)


#define NUM_LEDS noOfLEDs // Arduino NUM_LEDS yerine noOfLEDs kullanalım
#define CENTER_LED 21
#define BRIGHTNESS 50 // Parlaklık (0-255 ölçeğinde düşünülerek uygulanacak)

// Zorluk seviyeleri
#define EASY 1
#define MEDIUM 2
#define HARD 3
#define ON_SPEED 4
#define SONIC_SPEED 5
#define ROCKET_SPEED 6
#define LIGHT_SPEED 7
#define MISSION_IMPOSSIBLE 8

// Global oyun değişkenleri
int difficulty = EASY; // Başlangıç zorluğu EASY
bool wonThisRound = false;
int LEDaddress = 0;


#define BUTTON_GPIO_Port GPIOA
#define BUTTON_Pin GPIO_PIN_0
#define BUTTON_PRESSED_STATE GPIO_PIN_SET

uint32_t last_led_advance_time = 0;
// uint32_t button_press_time = 0; // Eğer kullanmıyorsanız bu satırı silebilirsiniz.
bool button_currently_pressed_flag = false;

// LED renklerini tutmak için bir yapı ve dizi
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBColor;

RGBColor led_strip[NUM_LEDS]; // LED'lerin renklerini saklayan gölge dizi
uint8_t global_brightness = BRIGHTNESS; // Global parlaklık

// HSV'den RGB'ye renk dönüşüm fonksiyonu
void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, RGBColor* rgb) {
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        rgb->r = v;
        rgb->g = v;
        rgb->b = v;
        return;
    }

    region = h / 43; // 256 / 6 ~= 42.66, so h is 0-255 for 6 regions
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0: rgb->r = v; rgb->g = t; rgb->b = p; break;
        case 1: rgb->r = q; rgb->g = v; rgb->b = p; break;
        case 2: rgb->r = p; rgb->g = v; rgb->b = t; break;
        case 3: rgb->r = p; rgb->g = q; rgb->b = v; break;
        case 4: rgb->r = t; rgb->g = p; rgb->b = v; break;
        default:rgb->r = v; rgb->g = p; rgb->b = q; break; // case 5
    }
}

// Gölge LED dizisindeki bir LED'in rengini ayarlar
void set_led_color_in_strip(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 0 && index < NUM_LEDS) {
        led_strip[index].r = r;
        led_strip[index].g = g;
        led_strip[index].b = b;
    }
}

// Tüm gölge LED dizisini belirli bir renkle doldurur
void fill_strip_solid(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < NUM_LEDS; i++) {
        set_led_color_in_strip(i, r, g, b);
    }
}

// Gölge LED dizisindeki renkleri parlaklık ayarıyla fiziksel LED'lere gönderir
void update_led_strip_to_physical_leds(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t r_adj = ((uint16_t)led_strip[i].r * global_brightness) / 255;
        uint8_t g_adj = ((uint16_t)led_strip[i].g * global_brightness) / 255;
        uint8_t b_adj = ((uint16_t)led_strip[i].b * global_brightness) / 255;

        // Kullanıcının setLED fonksiyonu (Pozisyon, Kırmızı, Yeşil, Mavi) parametrelerini alır.
        // WS2811 için renk sırası genellikle GRB'dir, bu yüzden setLED fonksiyonunun
        // bu sırayı doğru şekilde pwmData'ya yazdığını varsayıyoruz.
        // Arduino kodunda COLOR_ORDER RGB idi. FastLED bunu hallediyordu.
        // Sizin setLED fonksiyonunuzdaki döngüler: Yeşil, sonra Kırmızı, sonra Mavi bitlerini yazıyor.
        // Bu, setLED(pos, R, G, B) çağrıldığında, G'nin ilk, R'nin ikinci, B'nin üçüncü renk olarak
        // LED'e gönderileceği anlamına gelir. Bu WS2811 için standart bir GRB sıralamasıdır.
        // Eğer FastLED RGB sırasında gönderiyorsa ve WS2811'iniz RGB ise, setLED'e R,G,B vermek doğrudur.
        // Eğer WS2811'iniz GRB ise ve FastLED RGB gönderiyorsa, FastLED kendi içinde çeviriyordu.
        // Sizin setLED fonksiyonunuzun argümanları (Red, Green, Blue) ise ve WS2811'iniz GRB ise,
        // setLED(pos, Kirmizi, Yesil, Mavi) -> pwmData'ya Yesil, Kirmizi, Mavi bitlerini yazar.
        // Bu durumda bizim RGBColor {r,g,b} yapımızdaki değerleri doğrudan kullanabiliriz.
        setLED(i, r_adj, g_adj, b_adj);
    }
    ws2812Send();
}


// Oyun başlangıç ayarları
void game_setup(void) {
    // STM32 HAL başlatmaları (GPIO, TIM, DMA) main() içinde yapılmış olmalı.
    // Oyun mantığı için başlangıç ayarları:
    fill_strip_solid(0, 0, 0); // Tüm LED'leri siyah yap
    update_led_strip_to_physical_leds(); // Siyah rengi LED'lere gönder
    // Gerekirse STM32 UART başlatma kodları eklenebilir (Serial.begin eşdeğeri)
}

// Ana oyun döngüsü
void game_loop(void) {
    // GPIO_PinState currentButtonState = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin); // KALDIRILDI

    // OYUN SONU MANTIĞI
    // Interrupt, Playing'i false yapar. Bu blok şimdi Playing false olduğunda çalışır.
    if (!Playing) {
        // Bu blok, Arduino kodundaki `if (buttonState == HIGH)` içindeki kısma karşılık gelir.
        // Sadece bir kez çalışması gereken win/loss kontrolü
        if (CycleEnded) { // Interrupt tarafından true yapıldıysa
            // Hedef LED ve seçilen LED dışındaki tüm LED'leri kapat
            for (int i = 0; i < NUM_LEDS; i++) {
                set_led_color_in_strip(i, 0, 0, 0); // Siyah
            }
            set_led_color_in_strip(CENTER_LED, 255, 0, 0); // Merkez LED Kırmızı
            set_led_color_in_strip(LEDaddress, 0, 255, 0); // Durdurulan LED Yeşil
            update_led_strip_to_physical_leds();

            int diff = abs(CENTER_LED - LEDaddress);
            if (diff == 0) { // KAZANDI
                wonThisRound = true;
                if (difficulty != MISSION_IMPOSSIBLE) {
                    for (int i = 0; i < 2; i++) {
                        play_cylon_animation();
                    }
                } else { // MISSION_IMPOSSIBLE kazanıldı
                    for (int i = 0; i < 8; i++) {
                        play_cylon_animation();
                    }
                    difficulty = 0;
                }
                increase_game_difficulty();
                wonThisRound = false;
            } else { // KAYBETTİ
                HAL_Delay(1000); // Bu gecikme hala burada, oyun durmuşken sorun olmaz
                for (int i = 0; i < 2; i++) {
                    play_flash_animation();
                }
                difficulty = 1;
            }
            CycleEnded = false; // Bu kazanma/kaybetme durumu işlendi
        }

        // Buton bırakılana kadar bekle (yeniden başlatmak için)
        GPIO_PinState button_release_check = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin);
        // PULLDOWN konfigürasyonunda, bırakıldığında RESET (LOW) olur.
        if (button_release_check == GPIO_PIN_RESET) { // Buton bırakıldıysa
            Playing = true;       // Oyunu yeniden başlat
            LEDaddress = 0;       // LED adresini sıfırla
            HAL_Delay(10);       // Kısa bir bekleme

            fill_strip_solid(0,0,0);
            set_led_color_in_strip(CENTER_LED, 255, 0, 0);
            update_led_strip_to_physical_leds();
            // Bir sonraki game_loop turunda Playing true olacağı için LED'ler hareket etmeye başlayacak.
            HAL_Delay(10);
        }
    }

    // OYNANIŞ MANTIĞI
    if (Playing) {
        for (int i = 0; i < NUM_LEDS; i++) {
            set_led_color_in_strip(i, 0, 0, 0);
        }
        set_led_color_in_strip(CENTER_LED, 255, 0, 0);
        set_led_color_in_strip(LEDaddress, 0, 255, 0);
        update_led_strip_to_physical_leds();

        LEDaddress++;
        if (LEDaddress == NUM_LEDS) {
            LEDaddress = 0;
        }
        HAL_Delay(getTime(difficulty)); // Bu gecikme artık buton tepkisini etkilemeyecek

        // Oynanış sırasında butona basılırsa interrupt Playing'i false yapacak.
        // GPIO_PinState playingButtonCheck = HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin); // KALDIRILDI
        // if (playingButtonCheck == BUTTON_PRESSED_STATE) { // KALDIRILDI
        //     Playing = false;    // KALDIRILDI
        //     CycleEnded = true;  // KALDIRILDI
        // } // KALDIRILDI
    }
}
// Zorluğa göre LED hareket gecikmesini döndürür
int getTime(int diff_level) {
    int timeValue = 0;
    switch (diff_level) {
        case EASY: timeValue = 200; break;
        case MEDIUM: timeValue = 150; break;
        case HARD: timeValue = 100; break;
        case ON_SPEED: timeValue = 80; break;
        case SONIC_SPEED: timeValue = 60; break;
        case ROCKET_SPEED: timeValue = 50; break;
        case LIGHT_SPEED: timeValue = 30; break;
        case MISSION_IMPOSSIBLE: timeValue = 20; break;
        default: timeValue = 100; // Hata durumunda varsayılan
    }
    return timeValue;
}

// Kazanma durumunda zorluğu artırır
void increase_game_difficulty() {
    // Orijinal Arduino kodu: void increaseDifficulty() { if (difficulty != MISSION_IMPOSSIBLE && wonThisRound) { difficulty++; } }
    // Ve MISSION_IMPOSSIBLE kazanıldığında ana döngüde difficulty = 0; yapılıp sonra bu fonksiyon çağrılıyordu.
    // Bu mantık korunuyor:
    if (wonThisRound) { // Sadece kazanıldıysa zorluk değişir
        if (difficulty == 0) { // Bu, MISSION_IMPOSSIBLE'dan sonraki sıfırlamadır
            difficulty = 1; // EASY'ye geç
        } else if (difficulty < MISSION_IMPOSSIBLE) {
            difficulty++;
        }
        // Eğer difficulty zaten MISSION_IMPOSSIBLE ise ve kazanıldıysa,
        // game_loop içinde difficulty = 0 olarak ayarlandı. wonThisRound true olduğu için
        // bir sonraki adımda difficulty = 1 olacak.
    }
}


// Kaybetme durumunda LED animasyonu (iki kez kırmızı flaş)
void play_flash_animation() {
    uint16_t flash_duration = 300; // Kırmızı ışığın yanık kalma süresi (ms)
    uint16_t off_duration = 300;   // LED'lerin sönük kalma süresi (ms)

    // Birinci flaş
    fill_strip_solid(255, 0, 0); // Tüm LED'ler Kırmızı
    update_led_strip_to_physical_leds();
    HAL_Delay(flash_duration);

    fill_strip_solid(0, 0, 0);   // Tüm LED'ler Siyah (Sönük)
    update_led_strip_to_physical_leds();
    HAL_Delay(off_duration);

//    // İkinci flaş
//    fill_strip_solid(255, 0, 0); // Tüm LED'ler Kırmızı
//    update_led_strip_to_physical_leds();
//    HAL_Delay(flash_duration);

    fill_strip_solid(0, 0, 0);   // Tüm LED'ler Siyah (Sönük)
    update_led_strip_to_physical_leds();
    // Animasyon bittikten sonra LED'lerin sönük kalması için çok kısa bir bekleme,
    // bir sonraki LED güncellemesine kadar durumun korunmasına yardımcı olabilir.
    HAL_Delay(50);
}

// Tüm LED'lerin parlaklığını azaltır (FastLED'deki fadeall gibi)
void apply_fade_to_all_leds() {
    // Daha nazik bir soluklaştırma deneyin, kuyruk biraz daha uzun kalır.
    uint8_t fade_amount = 245; // 220 yerine 240, 245 gibi değerler deneyin
    for (int i = 0; i < NUM_LEDS; i++) {
        led_strip[i].r = (uint32_t)led_strip[i].r * fade_amount / 256;
        led_strip[i].g = (uint32_t)led_strip[i].g * fade_amount / 256;
        led_strip[i].b = (uint32_t)led_strip[i].b * fade_amount / 256;
    }
}

// Helper function to generate a color from a 0-255 position on a color wheel
// Pozisyon 0-255 arası bir değer alır ve bir gökkuşağı rengi döndürür.
void rgb_color_wheel(uint8_t wheel_pos, RGBColor* color) {
    wheel_pos = 255 - wheel_pos; // Renk döngüsünün yönünü değiştirebilirsiniz (isteğe bağlı)
    if (wheel_pos < 85) {
        color->r = 255 - wheel_pos * 3;
        color->g = 0;
        color->b = wheel_pos * 3;
    } else if (wheel_pos < 170) {
        wheel_pos -= 85;
        color->r = 0;
        color->g = wheel_pos * 3;
        color->b = 255 - wheel_pos * 3;
    } else {
        wheel_pos -= 170;
        color->r = wheel_pos * 3;
        color->g = 255 - wheel_pos * 3;
        color->b = 0;
    }
    // Not: Bu fonksiyonun ürettiği renkler tam parlaklıktadır (bir bileşen 255'e ulaşır).
    // Global parlaklık (global_brightness) daha sonra update_led_strip_to_physical_leds içinde uygulanacaktır.
    // Eğer bu animasyonun kendi parlaklığını ayrıca kısmak isterseniz,
    // burada üretilen r,g,b değerlerini bir miktar ölçekleyebilirsiniz.
    // Örn: color->r = color->r * anim_brightness_scale / 255; (anim_brightness_scale < 255)
}
// Kazanma durumunda Cylon LED animasyonu
// Kazanma durumunda Cylon LED animasyonu (RGB Renk Tekerleği ile)
void play_cylon_animation() {
    static uint8_t wheel_position = 0; // Renk tekerleğindeki mevcut pozisyon, çağrılar arasında devam eder.

    // Animasyon adımları arasındaki gecikme (ms).
    // Arduino'daki delay(10)'a karşılık gelir. Gerekirse artırılabilir.
    uint16_t step_delay = 10;

    // İleri yönde
    for (int i = 0; i < NUM_LEDS; i++) {
        RGBColor current_color;
        // Her LED için renk tekerleğinden bir sonraki rengi al
        // wheel_position uint8_t olduğu için 255'ten sonra otomatik olarak 0'a döner.
        rgb_color_wheel(wheel_position++, &current_color);

        set_led_color_in_strip(i, current_color.r, current_color.g, current_color.b);
        update_led_strip_to_physical_leds();
        // BUNU YAPINCA LEDLERIN RENKLERI BOZULUYO
//        apply_fade_to_all_leds(); // Kuyruk efekti için (fade_amount'ı deneyerek ayarlayın)
        HAL_Delay(step_delay);
    }

    // Geri yönde
    for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
        RGBColor current_color;
        rgb_color_wheel(wheel_position++, &current_color);

        set_led_color_in_strip(i, current_color.r, current_color.g, current_color.b);
        update_led_strip_to_physical_leds();
//        apply_fade_to_all_leds();
        HAL_Delay(step_delay);
    }
}

// -----------------------------------------------------------
// GEMINI OYUN KODU SONU
// -----------------------------------------------------------

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	LEDaddress = 0; // Oyun başlamadan hemen önce kesin olarak 0'a ayarla

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  game_setup();
  HAL_Delay(10); // WS2812B reset/hazırlık süresi için
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  game_loop();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 20-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 3-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == BUTTON_Pin) // Bizim butonumuz mu?
  {
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_button_interrupt_time > DEBOUNCE_TIME_MS)
    {
      last_button_interrupt_time = current_time;

      // Orijinal koddaki `if (currentButtonState == BUTTON_PRESSED_STATE && Playing)` mantığı
      if (Playing) // Eğer oyun zaten oynanıyorsa, butona basış oyunu durdurur
      {
        Playing = false;    // Oyunu durdur (bu bayrak game_loop tarafından okunacak)
        CycleEnded = true;  // Sonucun işlenmesi için bayrağı ayarla
      }
      // Butona basıldığında oyun durmuşsa ve yeniden başlatma bekleniyorsa,
      // bu durum game_loop içinde butonun BIRAKILMASIYLA ele alınacak.
      // Interrupt sadece "basılma" anını yakalar.
      // Eğer basılı tutarken bir şeylerin değişmesini istemiyorsanız, bu yeterli.
      // Eğer butona basar basmaz yeniden başlatma (Playing = true) olsaydı,
      // o zaman buraya ek bir mantık gerekirdi. Ama mevcut oyun akışınız buna uygun değil.
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
