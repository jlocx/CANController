#include <Arduino.h>
#include "TimerOne.h"
#include "BTL.h"
#include "config.h"

bool scaled_clock = LOW;
bool flag_finished_bit = false;
bool flag_sync = false;

void TimeQuantum()
{
    digitalWrite(TQ_CLK, !digitalRead(TQ_CLK));
    scaled_clock = HIGH;
}

BitTimingLogic::BitTimingLogic()
{
    this->hardsync = false;
    this->resync = false;
}

void BitTimingLogic::setup(uint32_t _TQ, int8_t _T1, int8_t _T2, int8_t _SJW)
{
    this->limit_TSEG1 = _T1;
    this->limit_TSEG2 = _T2;
    this->sjw = _SJW;
    this->frequency_divider(_TQ);
}

#if LOGGING
void logging_title()
{
    Serial.println(".----------.----------------.-----------.-----------.--------.----------.-------------.--------------.---------------.");
    Serial.println("| BUS IDLE | PREVIOUS INPUT | INPUT BIT | HARD SYNC | RESYNC |  STATE   | PHASE ERROR | SAMPLE POINT | WRITING POINT |");
    Serial.println("|----------|----------------|-----------|-----------|--------|----------|-------------|--------------|---------------|");
}

void logging_fotter()
{
    Serial.println("'----------'----------------'-----------'-----------'--------'----------'-------------'--------------'---------------'");
    Serial.println();
}
#endif
/*
void BitTimingLogic::BTL_BPL_interface(BTL_Data &btl_output, Bit_Stuffing_Writing_Data &bit_stuffing_writing_output, bool bus_idle_BPL, bool sampled_bit, bool &output_bit, bool &bus_idle, bool sample_point, bool writing_point)
{
    btl_output.sampled_bit = sampled_bit;
    btl_output.sample_point = sample_point;
    btl_output.writing_point = writing_point;
    output_bit = bit_stuffing_writing_output.output_bit;
    bus_idle = bus_idle_BPL;
}
*/
void BitTimingLogic::run(bool &tq, bool input_bit, bool write_bit, bool &sampled_bit, bool &output_bit, bool &bus_idle, bool &sample_point, bool &writing_point)
{
    static bool prev_input_bit = RECESSIVE;

    sampled_bit = input_bit;
    output_bit = write_bit;

    if (tq) {
        #if LOGGING
        logging_title();
        Serial.print("|     ");
        Serial.print(bus_idle, DEC);
        Serial.print("    |        ");
        Serial.print(prev_input_bit, DEC);
        Serial.print("       |     ");
        Serial.print(input_bit, DEC);
        Serial.print("     |     ");
        #endif
        
        this->edge_detector(prev_input_bit, input_bit, bus_idle);
        this->bit_segmenter(prev_input_bit, input_bit, sample_point, writing_point);

        #if LOGGING
        Serial.print("      |       ");
        Serial.print(sample_point, DEC);
        Serial.print("      |       ");
        Serial.print(writing_point, DEC);
        Serial.println("       |");
        logging_fotter();
        #endif

        #if SIMULATION
        if (flag_finished_bit) {
            prev_input_bit = input_bit;
        }
        #endif
        
        tq = false;
        //digitalWrite(TQ_CLK, LOW);
    }
}

bool BitTimingLogic::nextTQ(uint8_t pos, uint8_t &j, bool &tq)
{
    bool ret = false;
    
    if (flag_finished_bit) {
        flag_finished_bit = false;
        flag_sync = false;
        ret = true;
        j = 0;
    }
    else if (scaled_clock) {
        tq = true;
        
        #if !SIMULATION
        flag_sync = true;
        #endif
        
        #if SIMULATION
        
        if (j < pos) {
            j++;
        }
        else if (j == pos) {
            flag_sync = true;
            j++;
        }

        #endif

        scaled_clock = LOW;
    }

    return ret;
}

void BitTimingLogic::frequency_divider(uint32_t _TQ)
{
    Timer1.initialize(_TQ);
    Timer1.attachInterrupt(TimeQuantum);
}

void BitTimingLogic::edge_detector(bool &prev_input_bit, bool input_bit, bool &bus_idle)
{
    if (flag_sync) {
        if ((input_bit == DOMINANT) && (prev_input_bit == RECESSIVE)) {
            if (bus_idle) {
                this->hardsync = true;
                this->resync = false;
                
                #if SIMULATION
                bus_idle = false;
                #endif
            }
            else {
                this->hardsync = false;
                this->resync = true;
            }

            prev_input_bit = DOMINANT;
        }
        
        flag_sync = false;
    }
}

void initial_values_bit_segmenter(int8_t t1, int8_t t2, int8_t &p_count, bool &writing_point, bool &sample_point, uint8_t &state, int8_t &count_limit1, int8_t &count_limit2)
{
    p_count = 0;
    writing_point = HIGH;
    sample_point = LOW;
    state = TSEG1;
    count_limit1 = t1;
    count_limit2 = t2;
}

void BitTimingLogic::bit_segmenter(bool &prev_input_bit, bool input_bit, bool &sample_point, bool &writing_point)
{
    static uint8_t state = SYNC_SEG;
    static int8_t p_count = 0;
    static int8_t count_limit1 = 0, count_limit2 = 0;
    static bool sync = false;
    int8_t phase_error = 127;

    #if LOGGING
    Serial.print(this->hardsync, DEC);
    Serial.print("     |    ");
    Serial.print(this->resync, DEC);
    Serial.print("   | ");
    #endif

    if (this->hardsync) {
        this->hardsync = false;
        sync = true;
        digitalWrite(HARDSYNC, HIGH);

        digitalWrite(STATE_0, LOW);
        digitalWrite(STATE_1, LOW);

        #if LOGGING
        Serial.print("SYNC_SEG");
        Serial.print(" |     ");
        
        if (p_count >= 0 && p_count < 10) {
          Serial.print(" ");  
        }
        
        Serial.print(p_count, DEC);
        #endif
        
        initial_values_bit_segmenter(this->limit_TSEG1, 0, p_count, writing_point, sample_point, state, count_limit1, count_limit2);
    }
    else {
        digitalWrite(HARDSYNC, LOW);
        digitalWrite(RESYNC, LOW);

        switch(state)
        {
            case SYNC_SEG:
                digitalWrite(STATE_0, LOW);
                digitalWrite(STATE_1, LOW);

                initial_values_bit_segmenter(this->limit_TSEG1, 0, p_count, writing_point, sample_point, state, count_limit1, count_limit2);

                #if LOGGING
                Serial.print("SYNC_SEG");
                Serial.print(" |     ");
                
                if (p_count >= 0 && p_count < 10) {
                  Serial.print(" ");  
                }
                
                Serial.print(p_count, DEC);
                #endif
                
                break;
            case TSEG1:
                digitalWrite(STATE_0, HIGH);
                digitalWrite(STATE_1, LOW);

                #if LOGGING
                Serial.print(" TSEG_1 ");
                #endif

                writing_point = LOW;
                
                if (p_count < count_limit1) {
                    p_count++;
                }

                if (this->resync) {
                    this->resync = false;
                    sync = true;
                    digitalWrite(RESYNC, HIGH);

                    count_limit1 += min(this->sjw, p_count);
                }

                #if LOGGING
                Serial.print(" |     ");
                
                if (p_count >= 0 && p_count < 10) {
                  Serial.print(" ");  
                }
                
                Serial.print(p_count, DEC);
                #endif
                
                if (p_count == count_limit1) {
                    p_count = this->limit_TSEG2;
                    state = TSEG2;
                }

                break;
            case TSEG2:
                digitalWrite(STATE_0, LOW);
                digitalWrite(STATE_1, HIGH);

                if (p_count == this->limit_TSEG2) {
                    #if !SIMULATION
                    prev_input_bit = input_bit;
                    #endif
                    
                    sample_point = HIGH;
                }
                else {
                  sample_point = LOW;
                }

                #if LOGGING
                Serial.print(" TSEG_2 ");
                Serial.print(" |     ");
                
                if (p_count >= 0 && p_count < 10) {
                  Serial.print(" ");  
                }
                
                Serial.print(p_count, DEC);
                #endif

                if (this->resync) {
                    this->resync = false;
                    
                    digitalWrite(RESYNC, HIGH);

                    phase_error = min(this->sjw, -p_count);
                    count_limit2 -= phase_error;
                    sync = (p_count+1 == count_limit2);
                }
                
                if (p_count < count_limit2) {
                    p_count++;
                }
                
                if (p_count == count_limit2) {

                    if (phase_error == -p_count && !sync) {
                        initial_values_bit_segmenter(this->limit_TSEG1, 0, p_count, writing_point, sample_point, state, count_limit1, count_limit2);
                    }
                    else {
                        state = SYNC_SEG;
                    }

                    sync = false;
                    flag_finished_bit = true;
                }

                break;
        }
    }
}
