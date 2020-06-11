
if strcmp(pref, '')
    print('Error!')
end

n = importnodes(strcat(pref,'_node_key.csv'));
v = importvel(strcat(pref,'_vel_key.csv'));
um = importnvm(strcat(pref,'_NV_udp_conn.csv'));
dm = importnvm(strcat(pref,'_NV_data_link_conn.csv'));
%n = n(2:2:end-1);
%um = um(2 : 2 :end-1, :);
%dm = dm(2 : 2 :end-1, :);
eff = um./ dm;
create_connectivity_figure(v, n, dm, um, eff, 'n=');