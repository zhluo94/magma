#!/usr/bin/python3

"""
Calculate MOS score in a SIP call log and output a csv with the same filename
"""

import sys
import pandas as pd
import mos_score as mos
import time
from datetime import date
import statistics as st

def txt(filename = "", colname = "MOS Score"):
    if (filename):
        if len(filename.split('_')) == 4:
            log_date = filename.split('_')[3][:10] #YYYY-MM-DD
        else:
            # use today
            log_date = date.today().strftime("%Y-%m-%d")
        
        with open(filename, 'r') as fp:
            line = fp.readline()
            with open(filename + ".csv", 'w') as emtpy_csv:
                pass
            prevline = ""
            cnt = 1
            RX = False
            latency = ""
            jitter = ""
            loss = ""
            previous_pkt_loss = 0
            real_time_pkt_size = 0
            KBps = 0
            loss_time_list = []
            current_loss = 0
            current_loss_col = []
            current_latency = []
            current_jitter = []
            col = []
            col2 = []
            col3 = []
            col4 = []
            timestamp = 0
            last_total_pkt = 0
            current_total_pkt = 0
            measure_count = 0
            short_call_MOS = 0
            short_call_MOS_col = []
            culmnulative_short_call_loss = 0
            culmnulative_short_call_latency = []
            culmnulative_short_call_jitter = []

            while line:
                if ("pjsua_app_common.c !" in line):
                    log_time = line.split()[0]
                    time_struct = time.strptime(log_date + "-" + log_time,  "%Y-%m-%d-%H:%M:%S.%f")
                    timestamp = time.mktime(time_struct) + float('0.' + log_time.split('.')[-1])
                if ("Call time:" in line):
                    row = ""
                    latency = ""
                    jitter = ""
                    loss = ""
                if ("RX pt" in line):
                    RX = True
                if ("TX " in line):
                    RX = False
                if RX:
                    if ("total " in line):
                        split_line = line.split()
                        if ("Kpkt" in line):
                            total_pkts = float(split_line[1][0:-4]) * 1000
                            current_total_pkt = total_pkts
                        else:
                            total_pkts = float(split_line[1][0:-3])
                            current_total_pkt = total_pkts
                        recv = split_line[2]
                        if ("KB" in recv):
                            recv_size = float(recv[0:-2])
                        else:
                            recv_size = float(recv[0:-2]) * 1000
                        real_time_pkt_size = recv_size / total_pkts
                        KBps_string = split_line[6][5:9]
                        if KBps_string[3] == "K":
                            KBps = float(KBps_string[0:3])
                        else:
                            KBps = float(KBps_string)
                    if ("pkt loss" in line):
                        split_line = line.split()
                        loss = split_line[2][1:-3]
                        col2.append(str(loss))
                        raw_loss = int(split_line[1][5::])
                        if (raw_loss != previous_pkt_loss):
                            net_loss = raw_loss - previous_pkt_loss
                            current_loss = net_loss
                            current_loss_col.append(str(net_loss))
                            loss_time = net_loss * real_time_pkt_size / KBps
                            previous_pkt_loss = raw_loss
                        else:
                            loss_time = 0
                            current_loss = 0
                            current_loss_col.append("0")
                        loss_time_list.append(str(loss_time))
                    if ("jitter" in line):
                        split_line = line.split()
                        jitter = split_line[3]
                        current_jitter.append(str(split_line[5]))
                        culmnulative_short_call_jitter.append(float(split_line[5]))
                        col3.append(str(jitter))
                if ("RTT" in line):
                    split_line = line.split()
                    latency = split_line[4]
                    col4.append(str(latency))
                    current_latency.append(str(split_line[6]))
                    culmnulative_short_call_latency.append(float(split_line[6]))
                    mos_value = mos.calculate_mos(latency, jitter, loss, timestamp)
                    col.append(str(mos_value))
                    if (measure_count == 0):
                        short_call_MOS = mos_value
                    elif measure_count % 100 == 0:
                        total_pkts = current_total_pkt - last_total_pkt
                        short_call_loss = culmnulative_short_call_loss / total_pkts
                        short_call_MOS = mos.calculate_mos(st.mean(culmnulative_short_call_latency), st.mean(culmnulative_short_call_jitter), short_call_loss, timestamp)
                        short_call_MOS_col.append(str(short_call_MOS))
                        culmnulative_short_call_loss = 0
                        culmnulative_short_call_latency = []
                        culmnulative_short_call_jitter = []
                    else:
                        total_pkts = current_total_pkt - last_total_pkt
                        short_call_loss = culmnulative_short_call_loss / total_pkts
                        short_call_MOS = mos.calculate_mos(st.mean(culmnulative_short_call_latency), st.mean(culmnulative_short_call_jitter), short_call_loss, timestamp)
                        short_call_MOS_col.append(str(short_call_MOS))
                        culmnulative_short_call_loss += current_loss
                    short_call_MOS_col.append(str(short_call_MOS))
                    measure_count += 1
                        
                        
                prevline = line
                line = fp.readline()
                cnt += 1
            
            try:
                csv_input = pd.read_csv(filename + ".csv")
            except pd.errors.EmptyDataError:
                fr = pd.DataFrame(col, columns=[colname])
                fr["Packet Loss"] = pd.Series(col2)
                fr["Jitter"] = pd.Series(col3)
                fr["Latency"] = pd.Series(col4)
                fr["Loss Time"] = pd.Series(loss_time_list)
                fr["Short Call MOS"] = pd.Series(short_call_MOS_col)
                fr.to_csv(filename + ".csv", index=False)
            else:
                csv_input[colname] = pd.Series(col)
                csv_input["Packet Loss"] = pd.Series(col2)
                csv_input["Jitter"] = pd.Series(col3)
                csv_input["Latency"] = pd.Series(col4)
                csv_input["Loss Time"] = pd.Series(loss_time_list)
                csv_input["Short Call MOS"] = pd.Series(short_call_MOS_col)
                csv_input.to_csv(filename + ".csv", index=False)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if len(sys.argv) == 2:
            txt(sys.argv[1])
        elif len(sys.argv) == 3:
            txt(sys.argv[1], sys.argv[2])
        else:
            print("Incorrect Args")
    else:
        print("Please specify file name.")
    
