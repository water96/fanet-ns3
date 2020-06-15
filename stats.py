#! /usr/bin/env python3

import os
import sys
import time
import optparse
import subprocess
import signal
import shutil
import glob
import numpy as np

STRING_DELIM = "\n=====================================\n"

SEED_DIRECTORY_MASK = "run-*"
DEFAULT_OUTPUT = "results.csv"
DEFAULT_TARGET = "scalars-impl.csv"

def parse_cmd_args(argv):
    parser = optparse.OptionParser()
    parser.add_option("-d", "--directory", action="store", type="string", dest="dest_dir", default="",
                      metavar="DESTDIR",
                      help="Target directory where seeds store")

    parser.add_option("-o", "--output", action="store", type="string", dest="output", default=DEFAULT_OUTPUT,
                      metavar="OUTPUT",
                      help="out file")

    parser.add_option("-f", "--file-name", action="store", type="string", dest="target", default=DEFAULT_TARGET,
                      metavar="TARGET",
                      help="target file with scalars")

    parser.add_option("-b", "--bunch", action="store_true", dest="bunch", default=False,
                      help="Run thru all subdirectories in target directory")

    (options, args) = parser.parse_args()
    return options
    pass


def parse_csv_file(file):
  hdr = np.genfromtxt(file, delimiter=',', dtype='U', skip_footer=1)
  val = np.genfromtxt(file, delimiter=',', skip_header=1)
  d = dict(zip(hdr, val))
  return d

def calc_mean(val_vec):
  d = {}
  for k, v in val_vec.items():
    d[k] = v.mean()
  return d

def calc_std(val_vec):
  d = {}
  for k, v in val_vec.items():
    d[k] = v.std()
  return d

def write_csv_file(val_vec, file_name):
  h = [k for k in val_vec.keys()]
  hdr_str = ','.join(h)
  v = np.array(list(val_vec.values()))
  v.shape = len(val_vec), -1
  np.savetxt(file_name, v.T, header=hdr_str, comments="", fmt="%f", delimiter=',')

def write_data_csv_file(matrix, file_name):
  if len(matrix.shape) != 2:
    print("Bad shape %s!" % (str(matrix.shape)))
    return 1

  np.savetxt(file_name, matrix, fmt="%f", delimiter=',')
  return 0

def check_stats(val_vec):
  real_c = val_vec["udp_conn"]
  pot_c = val_vec["data_link_conn"]
  b = real_c <= pot_c
  print(b)
  return int(not b.all())

def get_scalar_vector_map(seed_dirs, target):
  val_vec = {}
  for d in seed_dirs:
    print("Process dir %s..." % d)
    t = os.path.join(d, target)
    print("Check result csv file %s..." % t)
    if not os.path.exists(t):
      print("This dir doesn't contain csv file, skip it...")
      continue

    print("Read csv file %s..." % t)
    d = parse_csv_file(t)
    print("Read %d headers: %s" % (len(d), str(d.keys())))
    for k, v in d.items():
      if not k in val_vec:
        val_vec[k] = np.array([])
      val_vec[k] = np.append(val_vec[k], v)

    print("Done with csv file %s...%s" % (t, STRING_DELIM))

  return val_vec

###################################################################33

def run_calc(dest_dir, target=DEFAULT_TARGET, output=DEFAULT_OUTPUT):
  if not os.path.isdir(dest_dir):
    print("Error! Target directory '%s' doesn't exist!" % dest_dir)
    return 1

  print("Go to target directory %s..." % dest_dir)
  print("Search experimentation directory by mask '%s'..." % SEED_DIRECTORY_MASK)
  init_dir = os.getcwd()
  os.chdir(dest_dir)
  seed_dirs = [d for d in glob.glob(SEED_DIRECTORY_MASK) if os.path.isdir(d)]
  if not seed_dirs:
    print("Error! Target directory '%s' doesn't contain seed directories!" % dest_dir)
    os.chdir(init_dir)
    return 1


  print("Found %d dirs!" % len(seed_dirs))
  val_vec = get_scalar_vector_map(seed_dirs, target)
  print("Done with csv files reading! Totally We have %d headers! Calculate mean and std statistical params..." % len(val_vec))
  
  mean_vec = calc_mean(val_vec)
  mean_vec = mean_vec + 0.000001
  std_vec = calc_std(val_vec)
  mean_file_name = "mean-" + output
  std_file_name = "std-" + output
  vec_file_name = "vec-" + output
  print("Save results to csv files %s and %s" % (mean_file_name, std_file_name)) 
  write_csv_file(mean_vec, mean_file_name)
  write_csv_file(std_vec, std_file_name)
  write_csv_file(val_vec, vec_file_name)

  rc = check_stats(val_vec)
  if rc:
    print("Warning! Real connectivity is more than potentail in %s!" % (dest_dir))

  test_arr = val_vec['data_link_conn']



  os.chdir(init_dir)
  return 0

if __name__ == '__main__':
    print('Script %s starts in directory %s%s' % (sys.argv[0], os.getcwd(), STRING_DELIM))

    options = parse_cmd_args(sys.argv)
    if options.bunch:
      init = os.getcwd()
      os.chdir(options.dest_dir)
      print('Script run thru subdirs in target directory %s' % (options.dest_dir))
      dirs = [d for d in os.listdir() if os.path.isdir(d)]
      print("Found %d dirs!" % len(dirs))
      for d in dirs:
        rc = run_calc(d, options.target, options.output)
      os.chdir(init)
    else:
      rc = run_calc(options.dest_dir, options.target, options.output)

    print('Done with rc = %d%s' % (rc, STRING_DELIM))
    sys.exit(rc)