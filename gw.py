import csv
import sys
import os
import subprocess

def list2leda(directory, txt_subdir):
	subdir = 'gw/'
	if not os.path.exists(directory+subdir):
		os.mkdir(directory+subdir, 0777)
	for f in os.listdir(directory+txt_subdir):
		if f.endswith(".txt"):
			f_without = f[:-4]
			commandline = './list2leda '+directory+txt_subdir+f+' >> '+directory+subdir+f_without+'.gw'
			subprocess.call(commandline, shell=True)

if __name__=="__main__":
	with open('enron_LDC_topic_full_result.csv', 'r') as main_read:
        reader = csv.reader(main_read, delimiter=',')
		next(reader)
		end_time = 0

		#check if time_int_dict created
		directory = 'fullenron_'+ str(k)+'/'
		csv_subdir = 'csv/'
		txt_subdir = 'txt/'
		if not os.path.exists(directory):
			os.mkdir(directory, 0777)
		if not os.path.exists(directory+csv_subdir):
			os.mkdir(directory+csv_subdir, 0777)
		if not os.path.exists(directory+txt_subdir):
			os.mkdir(directory+txt_subdir, 0777)
        
        for row in reader:
            sender = int(row[2])
			receiver = int(row[3])

            #check if sender is in dict
			if sender not in edges_dict.keys():
				edges_dict[sender] = {}

			#check if receiver is in dict of dict
			if receiver not in edges_dict[sender].keys():
				edges_dict[sender][receiver] = 0

			edges_dict[sender][receiver] += 1 #increment edge
			if edges_dict[sender][receiver] >= 1: 
                title = directory+txt_subdir+'enron_'+ str(k) +'_snap_'+str(t)+'.txt'
                title_csv = directory+csv_subdir+'enron_'+ str(k) +'_snap_'+str(t)+'.csv'

                txt_write = open(title, 'wb')#writer
                csv_write = open(title_csv, 'wb')#writer
                txt_write_newline = ''

                txt_writer = csv.writer(txt_write, delimiter=' ')
                csv_writer = csv.writer(csv_write, delimiter=',')

                multi_edge_arr = []

                csv_writer.writerow(["id", "time", "sender", "receiver", "sender_employee", "receiver_employee"])

            #write regular rows
            csv_writer.writerow([row[0], row[1], row[2], row[3], row[4], row[5]])
            #txt_writer.writerow([row[2], row[3]])
            edge = str(row[2])+str(row[3])
            if edge not in multi_edge_arr:
                txt_write.write(txt_write_newline+row[2]+' '+row[3])
                txt_write_newline = '\n'
                multi_edge_arr.append(edge)

            sender = str(row[2])
            receiver = str(row[3])
            if sender not in sender_list:
                sender_list.append(sender)
            # if receiver not in sender_list:
            # 	sender_list.append(receiver)

            #txt_writer.seek(-2, os.SEEK_END)
            #txt_writer.truncate()

	print('finished snapshot and k partitioning')
	print('sender list', len(sender_list))
	list2leda(directory, txt_subdir)

