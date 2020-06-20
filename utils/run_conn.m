addpath('/home/aleksey/work/export_fig')
if strcmp(pref, '')
    print('Error!')
end

n = importnodes(strcat(pref,'_node_key.csv'));
v = importvel(strcat(pref,'_vel_key.csv'));
um = importnvm(strcat(pref,'_NV_udp_conn.csv'));
dm = importnvm(strcat(pref,'_NV_data_link_conn.csv'));
eff = um./ dm;
create_connectivity_figure(v, n, dm, um, eff, 'n=', pref);