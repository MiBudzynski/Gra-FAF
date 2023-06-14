#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#include<ctype.h>
#include<string.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>

#define zdarzeniaX 250
#define zdarzeniaY 300
#define skala 3
#define X (zdarzeniaX * skala)
#define Y (zdarzeniaY * skala)
#define wykryte     1
#define wykonane 2
#define GraczX 48
#define GraczY 90
int PojazdyX[] = { 40, 40, 45 }; //CIEZAROWKA, CZERWONY, FIOLETOWY
int PojazdyY[] = { 88, 90, 85 };
#define predkosc 2
#define GraczXM (zdarzeniaX - GraczX)
#define GraczYM (zdarzeniaY - GraczY)

ALLEGRO_DISPLAY* disp;
ALLEGRO_BITMAP* buffer;
int rekord;
long klatka;
long wynik;
unsigned char klawisz[ALLEGRO_KEY_MAX];
bool start = false;

void wykrywanie_bledow(bool test, const char* description)
{
    if (test) return;

    printf("couldn't initialize %s\n", description);
    exit(1);
}

int losuj(int x, int y)
{
    return x + (rand() % (y - x));
}

void disp_in()
{
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);

    disp = al_create_display(X, Y);
    wykrywanie_bledow(disp, "display");

    buffer = al_create_bitmap(zdarzeniaX, zdarzeniaY);
    wykrywanie_bledow(buffer, "bitmap buffer");
}

void disp_przed_rysuj()
{
    al_set_target_bitmap(buffer);
}

void disp_po_rysuj()
{
    al_set_target_backbuffer(disp);
    al_draw_scaled_bitmap(buffer, 0, 0, zdarzeniaX, zdarzeniaY, 0, 0, X, Y, 0);

    al_flip_display();
}

void zdarzenie(ALLEGRO_EVENT* event)
{
    switch (event->type)
    {
    case ALLEGRO_EVENT_TIMER:
        for (int i = 0; i < ALLEGRO_KEY_MAX; i++)
            klawisz[i] &= wykryte;
        break;

    case ALLEGRO_EVENT_KEY_DOWN:
        klawisz[event->keyboard.keycode] = wykryte | wykonane;
        break;
    case ALLEGRO_EVENT_KEY_UP:
        klawisz[event->keyboard.keycode] &= wykonane;
        break;
    }
}

typedef struct GRAFIKA
{
    ALLEGRO_BITMAP* rzeczy;
    ALLEGRO_BITMAP* gracz;
    ALLEGRO_BITMAP* pojazd[3];
    ALLEGRO_BITMAP* wybuch[4];
} GRAFIKA;
GRAFIKA efekty;

void grafika_in()
{
    efekty.rzeczy = al_load_bitmap("animacja.png");
    efekty.gracz = al_load_bitmap("gracz.png");

    efekty.pojazd[0] = al_load_bitmap("ciezarowka.png");
    efekty.pojazd[1] = al_load_bitmap("czerwony.png");
    efekty.pojazd[2] = al_load_bitmap("fioletowy.png");

    efekty.wybuch[0] = al_create_sub_bitmap(efekty.rzeczy, 33, 10, 9, 9);
    efekty.wybuch[1] = al_create_sub_bitmap(efekty.rzeczy, 43, 9, 11, 11);
    efekty.wybuch[2] = al_create_sub_bitmap(efekty.rzeczy, 46, 21, 17, 18);
    efekty.wybuch[3] = al_create_sub_bitmap(efekty.rzeczy, 46, 40, 17, 17);
}

ALLEGRO_SAMPLE* huk;
ALLEGRO_SAMPLE* muzyka;

void audio_init()
{
    al_install_audio();
    al_init_acodec_addon();
    al_reserve_samples(128);

    huk = al_load_sample("explode.flac");
    muzyka = al_load_sample("muzyka.MP3");
}

typedef struct EFEKT
{
    int x, y;
    int ujecie;
    bool istnieje;
} EFEKT;

#define FX_N 128
EFEKT efekt[FX_N];

void efekt_init()
{
    for (int i = 0; i < FX_N; i++)
        efekt[i].istnieje = false;
}

void efekt_add(int x, int y)
{
    al_play_sample(huk, 0.75, 0, 1, ALLEGRO_PLAYMODE_ONCE, NULL);
    for (int i = 0; i < FX_N; i++)
    {
        if (efekt[i].istnieje)
            continue;

        efekt[i].x = x;
        efekt[i].y = y;
        efekt[i].ujecie = 0;
        efekt[i].istnieje = true;
        return;
    }
}

void efekt_ruch()
{
    for (int i = 0; i < FX_N; i++)
    {
        if (!efekt[i].istnieje)
            continue;

        efekt[i].ujecie++;

        if (efekt[i].ujecie == 8)
            efekt[i].istnieje = false;
    }
}

void efekt_wyswietl()
{
    for (int i = 0; i < FX_N; i++)
    {
        if (!efekt[i].istnieje)
            continue;

        ALLEGRO_BITMAP* bmp = efekty.wybuch[efekt[i].ujecie / 2];

        int x = efekt[i].x - (al_get_bitmap_width(bmp) / 2);
        int y = efekt[i].y - (al_get_bitmap_height(bmp) / 2);
        al_draw_bitmap(bmp, x, y, 0);
    }
}

typedef enum POJAZD_R
{
    ciezarowka,
    czerwony,
    fioletowy
} POJAZD_R;

typedef struct POJAZD
{
    int x;
    float y;
    POJAZD_R typ;
    bool istnieje;
} POJAZD;

#define POJAZD_N 16
POJAZD pojazd[POJAZD_N];

bool pokrycie(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
{
    if (ax1 > bx2) return false;
    if (ax2 < bx1) return false;
    if (ay1 > by2) return false;
    if (ay2 < by1) return false;

    return true;
}

bool uderza(int x, int y, int w, int h)
{
    for (int i = 0; i < POJAZD_N; i++)
    {
        if (!pojazd[i].istnieje)
            continue;

        int sw = PojazdyX[pojazd[i].typ];
        int sh = PojazdyY[pojazd[i].typ];

        if (pokrycie(x, y, x + w, y + h, pojazd[i].x, pojazd[i].y, pojazd[i].x + sw, pojazd[i].y + sh))
        {
            efekt_add(pojazd[i].x + (sw / 2), pojazd[i].y + (sh / 2));
            pojazd[i].istnieje = false;
            return true;
        }
    }

    return false;
}

int korek(int x)
{
    for (int i = 0; i < POJAZD_N; i++)
    {
        if (!pojazd[i].istnieje || x == i)
            continue;

        int sw = PojazdyX[pojazd[i].typ];
        int sh = PojazdyY[pojazd[i].typ];
        int xw = PojazdyX[pojazd[i].typ];
        int xh = PojazdyY[pojazd[i].typ];

        if (pokrycie(pojazd[x].x, pojazd[x].y, pojazd[x].x + xw, pojazd[x].y + xh, pojazd[i].x, pojazd[i].y, pojazd[i].x + sw, pojazd[i].y + sh))
            if (pojazd[i].typ < pojazd[x].typ)
                return i;
            else
                return x;
    }

    return -1;
}

void pojazd_in()
{
    for (int i = 0; i < POJAZD_N; i++)
        pojazd[i].istnieje = false;
}

void pojazd_ruch()
{
    int seria =
        (klatka % 250)
        ? 0
        : losuj(2, 4)
        ;
    int pozycja;

    for (int i = 0; i < POJAZD_N; i++)
    {
        if (!pojazd[i].istnieje)
        {
            if (seria > 0)
            {
                pozycja = losuj(0, 5);

                pojazd[i].x = 50 * pozycja;
                pojazd[i].y = -100;
                pojazd[i].typ = (losuj(0, 3));
                pojazd[i].istnieje = true;

                seria--;

                if (korek(i) != -1)
                {
                    pojazd[i].x = 0;
                }
                else
                {
                    continue;
                }
                if (korek(i) != -1)
                {
                    pojazd[i].x = 50;
                }
                else
                {
                    continue;
                }
                if (korek(i) != -1)
                {
                    pojazd[i].x = 100;
                }
                else
                {
                    continue;
                }
                if (korek(i) != -1)
                {
                    pojazd[i].x = 150;
                }
                else
                {
                    continue;
                }
                if (korek(i) != -1)
                {
                    pojazd[i].x = 200;
                }
                else
                {
                    continue;
                }
            }
            continue;
        }
        if (korek(i) == -1)
        {
            switch (pojazd[i].typ)
            {
            case ciezarowka:
                if (!(klatka % 2))
                    pojazd[i].y += 1.5;
                break;
            case czerwony:
                pojazd[i].y++;
                break;

            case fioletowy:
                pojazd[i].y += 2;
                break;
            }
        }
        else
        {
            switch (pojazd[korek(i)].typ)
            {
            case ciezarowka:
                if (!(klatka % 2))
                    pojazd[i].y++;
                break;
            case czerwony:
                pojazd[i].y++;
                break;

            case fioletowy:
                pojazd[i].y += 1.5;
                break;
            }
        }

        if (pojazd[i].y >= zdarzeniaY)
        {
            pojazd[i].istnieje = false;
            continue;
        }
    }
}

void pojazd_wyswietl()
{
    for (int i = 0; i < POJAZD_N; i++)
    {
        if (!pojazd[i].istnieje)
            continue;

        al_draw_bitmap(efekty.pojazd[pojazd[i].typ], pojazd[i].x, pojazd[i].y, 0);
    }
}

typedef struct GRACZ
{
    int x, y;
    int zycia;
    int odrodzenie;
} GRACZ;
GRACZ gracz;

void gracz_in()
{
    gracz.x = (zdarzeniaX / 2) - (GraczX / 2);
    gracz.y = (zdarzeniaY / 2) - (GraczY / 2);
    gracz.zycia = 2;
    gracz.odrodzenie = 120;
}

void gracz_ruch()
{
    if (gracz.zycia < 0)
        return;

    if (klawisz[ALLEGRO_KEY_LEFT])
        gracz.x -= predkosc;
    if (klawisz[ALLEGRO_KEY_RIGHT])
        gracz.x += predkosc;
    if (klawisz[ALLEGRO_KEY_UP])
        gracz.y -= predkosc;
    if (klawisz[ALLEGRO_KEY_DOWN])
        gracz.y += predkosc;

    if (gracz.x < 0)
        gracz.x = 0;
    if (gracz.y < 0)
        gracz.y = 0;
    if (gracz.x > GraczXM)
        gracz.x = GraczXM;
    if (gracz.y > GraczYM)
        gracz.y = GraczYM;

    if (gracz.odrodzenie)
        gracz.odrodzenie--;
    else
    {
        if (uderza(gracz.x, gracz.y, GraczX, GraczY))
        {
            int x = gracz.x + (GraczX / 2);
            int y = gracz.y + (GraczY / 2);
            efekt_add(x, y);
            efekt_add(x + 4, y + 2);
            efekt_add(x - 2, y - 4);
            efekt_add(x + 1, y - 5);

            gracz.x = (zdarzeniaX / 2) - (GraczX / 2);
            gracz.y = 200;

            gracz.zycia--;
            gracz.odrodzenie = 180;
        }
    }
}

void gracz_wyswietl()
{
    if (gracz.zycia < 0)
        return;
    if (((gracz.odrodzenie / 2) % 3) == 1)
        return;

    al_draw_bitmap(efekty.gracz, gracz.x, gracz.y, 0);
}

ALLEGRO_FONT* font;

void napis_in()
{
    font = al_create_builtin_font();
    wykrywanie_bledow(font, "font");

    wynik = 0;
}

void napis_ruch()
{
    if (klatka % 500)
        wynik++;
}

void napis_wyswietl()
{
    if (wynik > rekord)
        rekord = wynik;

    al_draw_textf(font, al_map_rgb_f(1, 1, 1), 1, 1, 0, "%06ld", rekord);
    al_draw_textf(font, al_map_rgb_f(1, 1, 1), 1, 10, 0, "%06ld", wynik);
    al_draw_textf(font, al_map_rgb_f(1, 1, 1), 1, 20, 0, "%01ld", (gracz.zycia + 1));

    if (gracz.zycia < 0)
    {
        al_draw_text(font, al_map_rgb_f(1, 1, 1), zdarzeniaX / 2, zdarzeniaY / 2, ALLEGRO_ALIGN_CENTER, "G A M E  O V E R");
        start = false;
    }
}

typedef struct LINIE
{
    float y, x;
} LINIE;

#define LINIE_N 16
LINIE linie[LINIE_N];

void Linie_in()
{
    int pom = 0;
    for (int i = 0; i < LINIE_N; i++)
    {
        linie[i].x = 50 + (i % 4) * 50;
        linie[i].y = pom;
        if (i % 4 == 3)
            pom += 80;
    }
}

void Linie_ruch()
{
    for (int i = 0; i < LINIE_N; i++)
    {
        linie[i].y += 0.5;
        if (linie[i].y >= zdarzeniaY)
            linie[i].y = 0;
    }
}

void Linie_wyswietl()
{
    for (int i = 0; i < LINIE_N; i++)
    {
        al_draw_pixel(linie[i].x, linie[i].y, al_map_rgb_f(1, 1, 1));
        al_draw_line(linie[i].x, linie[i].y, linie[i].x, linie[i].y + 20, al_map_rgb_f(1, 1, 1), 2);
    }
}

int main()
{
    wykrywanie_bledow(al_init(), "allegro");
    wykrywanie_bledow(al_install_keyboard(), "keyboard");
    wykrywanie_bledow(al_install_mouse(), "mouse");

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    wykrywanie_bledow(timer, "timer");

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    wykrywanie_bledow(queue, "queue");

    ALLEGRO_MOUSE_STATE myszka;

    disp_in();

    audio_init();

    wykrywanie_bledow(al_init_image_addon(), "image");
    grafika_in();

    napis_in();

    wykrywanie_bledow(al_init_primitives_addon(), "primitives");

    wykrywanie_bledow(al_install_audio(), "audio");
    wykrywanie_bledow(al_init_acodec_addon(), "audio codecs");
    wykrywanie_bledow(al_reserve_samples(16), "reserve samples");

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    memset(klawisz, 0, sizeof(klawisz));
    efekt_init();
    gracz_in();
    pojazd_in();
    Linie_in();

    klatka = 0;
    wynik = 0;

    bool done = false;
    ALLEGRO_EVENT event;

    FILE *in, *out;
    in = fopen("rekord.txt", "r");
    fscanf(in, "%d", &rekord);
    fclose(in);

    al_init_font_addon();
    al_init_ttf_addon();
    ALLEGRO_FONT* czcionka = NULL;
    czcionka = al_load_ttf_font("arial.ttf", 72, 0);

    bool go = false;

    al_play_sample(muzyka, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
    al_start_timer(timer);

    while (1)
    {
        al_wait_for_event(queue, &event);

        if (start)
        {
            switch (event.type)
            {
            case ALLEGRO_EVENT_TIMER:
                efekt_ruch();
                gracz_ruch();
                pojazd_ruch();
                napis_ruch();
                Linie_ruch();

                if (klawisz[ALLEGRO_KEY_ESCAPE])
                    done = true;

                klatka++;
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                done = true;
                break;
            }

            if (done)
                break;

            zdarzenie(&event);

            disp_przed_rysuj();
            al_clear_to_color(al_map_rgb(0, 0, 0));

            Linie_wyswietl();
            pojazd_wyswietl();
            efekt_wyswietl();
            gracz_wyswietl();
            napis_wyswietl();

            disp_po_rysuj();
            go = false;
        }
        else
        {
            out = fopen("rekord.txt", "w");
            fprintf(out, "%d", rekord);
            fclose(out);

            switch (event.type)
            {
            case ALLEGRO_EVENT_TIMER:
                if (klawisz[ALLEGRO_KEY_ESCAPE])
                    done = true;
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                done = true;
                break;
            }

            if (done)
                break;

            al_draw_filled_rectangle(150, 300, 600, 650, al_map_rgba_f(1, 1, 1, 1));

            al_get_mouse_state(&myszka);
            if ((event.mouse.x > 200 && event.mouse.x < 550) && (event.mouse.y > 425 && event.mouse.y < 525) && al_mouse_button_down(&myszka, 1))
            {
                start = true; wynik = 0; gracz.zycia = 2;
            }
            else if((event.mouse.x > 200 && event.mouse.x < 550) && (event.mouse.y > 425 && event.mouse.y < 525))
                go = true;

            if (go)
            {
                al_draw_filled_rectangle(200, 425, 550, 525, al_map_rgba_f(0, 100, 0, 1));
                al_draw_textf(czcionka, al_map_rgb_f(1, 1, 1), 313, 435, 0, "%s", "GO!");
            }
            else
            {
                al_draw_filled_rectangle(200, 425, 550, 525, al_map_rgba_f(200, 0, 0, 1));
                al_draw_textf(czcionka, al_map_rgb_f(1, 1, 1), 233, 435, 0, "%s", "READY?");
            }

            zdarzenie(&event);

            al_flip_display();
        }
    }

    al_destroy_font(font);
    al_destroy_sample(huk);
    al_destroy_sample(muzyka);
    al_destroy_bitmap(efekty.gracz);
    al_destroy_bitmap(efekty.pojazd[0]);
    al_destroy_bitmap(efekty.pojazd[1]);
    al_destroy_bitmap(efekty.pojazd[2]);
    al_destroy_bitmap(efekty.wybuch[0]);
    al_destroy_bitmap(efekty.wybuch[1]);
    al_destroy_bitmap(efekty.wybuch[2]);
    al_destroy_bitmap(efekty.wybuch[3]);
    al_destroy_bitmap(efekty.rzeczy);
    al_destroy_bitmap(buffer);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}