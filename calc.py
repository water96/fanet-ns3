#! /usr/bin/env python3

import os
import sys
import time
import optparse
import subprocess
import signal
import shutil
import glob
import re
import numpy as np
import stats as st

STRING_DELIM = "\n=====================================\n"

DEFAULT_TARGET = "mean-results.csv"
DEFAULT_OUTPUT = "out"

def parse_cmd_args(argv):
    parser = optparse.OptionParser()
    parser.add_option("-d", "--directory", action="store", type="string", dest="dest_dir", default="",
                      metavar="DESTDIR",
                      help="Target directory where experiments results store")

    parser.add_option("-o", "--output", action="store", type="string", dest="output", default=DEFAULT_OUTPUT,
                      metavar="OUTPUT",
                      help="Out directory")

    parser.add_option("-f", "--file-name", action="store", type="string", dest="target", default=DEFAULT_TARGET,
                      metavar="TARGET",
                      help="target file with scalars")

    parser.add_option("-M", "--mobility", action="store", type="string", dest="mobility", default="",
                      metavar="MOB",
                      help="Names of mobililty")

    parser.add_option("-R", "--routing", action="store", type="string", dest="routing", default="",
                      metavar="ROUTING",
                      help="Names of routing")

    parser.add_option("-s", "--scalars", action="append", type="string", dest="scalars",
                      metavar="ROUTING",
                      help="Names of routing")

    (options, args) = parser.parse_args()
    return (options, args)

###################################################################33

def get_all_valid_dirs(dest_dir):
  exp_dir_mask = r'^.*-.*-.*-.*$'
  print("Scan %s directory by mask %s..." % (dest_dir, exp_dir_mask))

  exp_dirs = []
  a = os.listdir(dest_dir)
  for d in a:
    if not os.path.isdir(os.path.join(dest_dir, d)):
      print("Check dir %s... Bad! It is not directory, skip i!..." % (d))
    elif re.fullmatch(exp_dir_mask, d) == None:
      print("Check dir %s... Bad! It doesn't match the pattern %s" % (d, exp_dir_mask))
    else:
      print("Check dir %s... Good!" % (d))
      exp_dirs.append(d)

  print("\t---> Found: %d, skipped: %d <---" % (len(exp_dirs), len(a) - len(exp_dirs)))
  return exp_dirs

def get_scalars_map(dest_dir, exp_list = [], target=DEFAULT_TARGET):
  if len(exp_list) == 0:
    exp_list = get_all_valid_dirs(dest_dir)

  scalars_map = {}
  for e in exp_list:
    local = os.path.join(dest_dir, e)
    f = os.path.join(local, target)
    print("Check file %s in directory %s..." % (target, local))
    if os.path.exists(f):
      print("\t\t...Ok!")
    else:
      print("\t\t...Bad! Skip dir %s" % local)
      continue

    print("Open file %s" % f)
    d = st.parse_csv_file(f)
    print("Found %d keys: %s\nDone!" % (len(d), str(d.keys())))
    scalars_map[e] = d

  return scalars_map

def get_all_scalars(dest_dir, dir_list = [], target=DEFAULT_TARGET):
  if len(dir_list) == 0:
    dir_list = get_all_valid_dirs(dest_dir)

  scalars = set()
  for d in dir_list:
    local = os.path.join(dest_dir, d)
    f = os.path.join(local, target)
    print("Check file %s in directory %s..." % (target, local))
    if os.path.exists(f):
      print("\t\t...Ok!")
    else:
      print("\t\t...Bad! Skip dir %s" % local)
      continue

    print("Open file %s" % f)
    d = st.parse_csv_file(f)
    print("Found %d keys: %s" % (len(d), str(d.keys())))
    [scalars.add(s) for s in d.keys()]

  return list(scalars)

def get_mobs_vector(dir_list):
  comps_list = [d.split("-") for d in dir_list if len(d.split("-")) == 4]
  m_key_str = {}
  for c in comps_list:
    m = c[0]
    m_key_str[m] = c[0]

  return m_key_str

def get_routing_vector(dir_list):
  comps_list = [d.split("-") for d in dir_list if len(d.split("-")) == 4]
  r_key_str = {}
  for c in comps_list:
    r = c[1]
    r_key_str[r] = c[1]

  return r_key_str

def get_node_vector(dir_list):
  comps_list = [d.split("-") for d in dir_list if len(d.split("-")) == 4]
  n_key_str = {}
  for c in comps_list:
    n = np.int(re.sub(r'n.*$', "", c[2]))
    n_key_str[n] = c[2]

  return n_key_str

def get_velocity_vector(dir_list):
  comps_list = [d.split("-") for d in dir_list if len(d.split("-")) == 4]
  v_key_str = {}
  for c in comps_list:
    v = np.float(re.sub(r'v.*$', "", c[3]))
    v_key_str[v] = c[3]

  return v_key_str

def get_experiments(dir_list, m = ".*", r = ".*", n = ".*", v = ".*",):
  exp_dir_mask = r'^' + m + '-' + r + '-' + n + '-' + v + '$'
  print("Scan list directory by mask %s..." % (exp_dir_mask))

  exp_dirs = []
  for d in dir_list:
    if re.fullmatch(exp_dir_mask, d) != None:
      exp_dirs.append(d)

  print("\t---> Found: %d, skipped: %d <---" % (len(exp_dirs), len(dir_list) - len(exp_dirs)))
  return exp_dirs

#suppose that each node has the same number of velocities experiment
#else error
def get_NV_matrix(exp_res_map, scalars_names):
  if len(scalars_names) == 0:
    print("Error! Scalars names list is empty...")
    return (None, None, None)

  print("Scalars names list: %s" % str(scalars_names))

  n_map = get_node_vector(exp_res_map.keys())  
  print("Found %d node experiments in list. Sort it..." % (len(n_map)))
  n_vec = np.sort(np.array(list(n_map.keys()), dtype=int))
  print("Results node vector:\n\t%s" % (str(n_vec)))

  v_map = get_velocity_vector(exp_res_map.keys())
  print("Found %d velocity experiments in list. Sort it..." % (len(v_map)))
  v_vec = np.sort(np.array(list(v_map.keys()), dtype=float))
  print("Results velocity vector:\n\t%s" % (str(v_vec)))

  print("For each scalar create zeroed maxtrix %dx%d..." % (n_vec.size, v_vec.size))
  s_name_matrix_map = {}
  for s in scalars_names:
    s_name_matrix_map[s] = np.zeros((n_vec.size, v_vec.size))

  n_cnter = int(0)
  for node in n_vec:
    print("Start process experiments for n = %d..." % (node))
    v_cnter = int(0)
    for vel in v_vec:
      print("\tSearch experiment for n = %d, v = %f..." % (node, vel))
      exp = get_experiments(exp_res_map.keys(), n = n_map[node], v = v_map[vel])
      if len(exp) != 1:
        print("\tError! Experiment for n = %d, v = %f not found!" % (node, vel))
        return (n_vec, v_vec, s_name_matrix_map)

      s_d = exp_res_map[exp[0]]
      print("\tFound scalars map %s...\nInsert values to (%d; %d) position in matrix..." % (str(s_d), n_cnter, v_cnter))

      for s in s_name_matrix_map.keys():
        s_name_matrix_map[s][n_cnter][v_cnter] = s_d[s] 

      v_cnter += 1

    n_cnter += 1

  return (n_vec, v_vec, s_name_matrix_map)


##    Commands
def scan_experiments(dest_dir, target=DEFAULT_TARGET):

  return 1

def create_NV_matrix(dest_dir, mobility, routing, scalars_list, target=DEFAULT_TARGET, output=DEFAULT_OUTPUT):
  print("Create NV matrix command start!")

  if not os.path.isdir(dest_dir):
    print("Check direcotry %s... Error: direcotry doesn't exist" % dest_dir)
    return 1

  print("Get full experiments list...")
  all_exps = get_all_valid_dirs(dest_dir)
  if len(all_exps) == 0:
    print("Error! Direcotry %s doesn't contain any experiments..." % dest_dir)
    return 1

  print("Get experiments with M = %s and R = %s..." % (mobility, routing))
  target_exp = get_experiments(all_exps, m = mobility, r = routing)
  if len(target_exp) == 0:
    print("Error! Cannot find any exp with M = %s and R = %s..." % (mobility, routing))
    return 1

  if scalars_list == None or len(scalars_list) == 0:
    print("List of scalars is empty. Populate it...")
    scalars_list = get_all_scalars(dest_dir, target_exp, target)

  if len(scalars_list) == 0:
    print("Error! List of scalars is empty")
    return 1

  print("Read scalar file %s in each directory..." % target)
  exp_scalar_map = get_scalars_map(dest_dir, target_exp, target)
  if len(exp_scalar_map) == 0:
    print("Error! List of scalars is empty")
    return 1

  (n_vec, v_vec, s_name_matrix_map) = get_NV_matrix(exp_scalar_map, scalars_list)
  if len(n_vec) == 0 or len(v_vec) == 0 or len(s_name_matrix_map) == 0:
    print("Error! Cannot create nv matrix!")
    return 1

  print("NV matrix for M = %s and R = %s successfully created..." % (mobility, routing))
  print("Store results in output directory %s..." % output)
  if not os.path.isdir(output):
    os.mkdir(output)

  n_key_file = os.path.join(output, mobility + "_" + routing + "_node_key.csv")
  v_key_file = os.path.join(output, mobility + "_" + routing + "_vel_key.csv")
  print("Save node and velocity vectors to:\n\tnodes: %s\n\tvelocities: %s" % (n_key_file, v_key_file))

  st.write_csv_file({"n": n_vec}, n_key_file)
  st.write_csv_file({"v": v_vec}, v_key_file)

  for (scalar, matrix) in s_name_matrix_map.items():
    outfile = os.path.join(output, mobility + "_" + routing + "_" + "NV_" + scalar + ".csv")
    print("Save matrix for scalar %s to:\n\t%s" % (scalar, outfile))
    if st.write_data_csv_file(matrix, outfile) != 0:
      print("Error! Cannot write %s scalar.\nMatrix:\n" % (scalar, str(matrix)))

  return 0
  pass


if __name__ == '__main__':
    print('Script %s starts in directory %s%s' % (sys.argv[0], os.getcwd(), STRING_DELIM))

    (options, args) = parse_cmd_args(sys.argv)

    print(options)
    print(args)

    rc = 0
    if len(args) != 1:
      print("Error! should be one command given...")
      rc = 1
    elif args[0] == "scan":
      rc = scan_experiments(options.dest_dir, options.target)
    elif args[0] == "matrix":
      rc = create_NV_matrix(options.dest_dir, options.mobility, options.routing, options.scalars, options.target, options.output)
    else:
      print("Error! Wrong command!")
      rc = 1

    print('Done with rc = %d%s' % (rc, STRING_DELIM))
    sys.exit(rc)