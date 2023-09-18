import pandas as pd
import os

os.chdir('../build/logs')
dfs = []
for file in os.listdir():
    if len(file) > 4 and file[-4:] == '.csv' and file != 'merged.csv':
        dfs.append(pd.read_csv(file, sep='|', index_col=None))
merged = pd.concat(dfs)
merged = merged.sort_values('timestamp')
merged.to_csv('merged.csv', sep='|', index=False)
os.chdir('../../mixnet')
