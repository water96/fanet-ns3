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

MODEL_EXEC = ""

NODES_STRING_DEFAULT = "2..2..20"
SPEEDS_STRING_DEFAULT = "100..20..200"
RUNS_STRING_DEFAULT = "1..1..10"

MOB_STRING_DEFAULT = "GM,CONST,RWP"
ROUTING_STRING_DEFAULT = "AODV"

def create_sym_arr(s):
  arr = s.split(",")
  return arr

def create_range_arr(r, type=int):
  arr = r.split("..")
  a = []
  if len(arr) == 3:
    a = list(np.arange(type(arr[0]), type(arr[2]) + type(arr[1]), type(arr[1]), dtype=type))
  elif len(arr) == 1:
    a.append(type(arr[0]))
  return list(map(str,a))

def parse_param_string(s):
  a = s.split("-")
  return tuple(a)

def parse_cmd_args(argv):
  parser = optparse.OptionParser()
  parser.add_option("-d", "--directory", action="store", type="string", dest="out_dir", default="out",
                  metavar="out",
                  help="Output directory")

  parser.add_option("-l", "--log-file", action="store", type="string", dest="log_file_name", default="",
                  metavar="",
                  help="Log file name")

  parser.add_option("-N", "--nodes", action="store", type="string", dest="nodes", default=NODES_STRING_DEFAULT,
                  metavar=NODES_STRING_DEFAULT,
                  help="Number of nodes")

  parser.add_option("-r", "--runs", action="store", type="string", dest="runs", default=RUNS_STRING_DEFAULT,
                  metavar=RUNS_STRING_DEFAULT,
                  help="Run indexes")

  parser.add_option("-V", "--speeds", action="store", type="string", dest="speeds", default=SPEEDS_STRING_DEFAULT,
                  metavar=SPEEDS_STRING_DEFAULT,
                  help="Number of speeds")

  parser.add_option("-M", "--mobility", action="store", type="string", dest="mobility", default=MOB_STRING_DEFAULT,
                  metavar=MOB_STRING_DEFAULT,
                  help="Names of mobililty")

  parser.add_option("-R", "--routing", action="store", type="string", dest="routing", default=ROUTING_STRING_DEFAULT,
                  metavar=ROUTING_STRING_DEFAULT,
                  help="Names of routing")

  parser.add_option("-j", "--jobs", action="store", type="string", dest="jobs", default="10",
                metavar="JOBS",
                help="Number of jobs")

  parser.add_option("-b", "--bunch-string", action="store", dest="mod_str", default="",
                  help="Compact string representation in format M-R-N-V-S")

  (options, args) = parser.parse_args()
  return options
  pass

def create_arg_list(param_arr, out_dir, jobs):
  a = ["parallel" , "-j" + jobs]
  exec_name = MODEL_EXEC + " --mobility={1} --routing={2} --nodes={3} --speed={4} --run={5} --out-dir=" + out_dir
  a.append(exec_name)
  for p in param_arr:
    a.append(":::")
    a.extend(p)

  return a

def run_all(param_tuple, out_dir, jobs, log=""):
  print("Create args list...")

  args = create_arg_list(list(param_tuple), out_dir, jobs)

  total = 1
  for a in param_tuple:
    total = total * len(a)

  print("Total simulations will be peformed: %d..." % total)
  print("Run simulation!\nCommand: %s" % " ".join(args))
  t1 = time.time()
  print("Start at %s" % time.ctime(t1))
  if log != "":
    with open(log, 'w') as f:
      subprocess.call(args, stdout=f)
  else:
    subprocess.call(args)

  t2 = time.time()
  print("End at %s", time.ctime(t2))
  print("\t----> Elapsed time: %f seconds! <----" % (t2 - t1))
  return 0


if __name__ == '__main__':
  print('Script %s starts in directory %s%s' % (sys.argv[0], os.getcwd(), STRING_DELIM))

  os.chdir(os.path.dirname(sys.argv[0]))
  MODEL_EXEC = os.path.join(os.getcwd(), "fanet", "fanet")

  print("%s" % MODEL_EXEC)

  opt = parse_cmd_args(sys.argv)

  if opt.mod_str != "":
    print('Parameters string is not empty. Parse it and ignore other')
    t = parse_param_string(opt.mod_str)
    if len(t) != 5:
      print('Error! Invalid param string %s!' % opt.mod_str)
      print('Done with rc = %d%s' % (1, STRING_DELIM))
      sys.exit(1)

    (opt.mobility, opt.routing, opt.nodes, opt.speeds, opt.runs) = t

  m = create_sym_arr(opt.mobility)
  r = create_sym_arr(opt.routing)

  n = create_range_arr(opt.nodes)
  v = create_range_arr(opt.speeds, type=float)
  s = create_range_arr(opt.runs)

  print('Pararms parsing done!')
  t = (m, r, n, v, s)
  print('\t m: %s\n\t r: %s\n\t n: %s\n\t v: %s\n\t s: %s' % t)

  opt.out_dir = os.path.abspath(opt.out_dir)
  rc = run_all(t, opt.out_dir, opt.jobs, opt.log_file_name)
  print('Done cript %s with rc = %d%s' % (sys.argv[0], rc, STRING_DELIM))
  sys.exit(rc)