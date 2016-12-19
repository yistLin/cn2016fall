#!/usr/local/bin/env python3
# robot.py
# author: B03902048
# purpose: Computer Network 2016 Fall HW1

import socket
import re
from random import randint
from bs4 import BeautifulSoup
import requests

# settings
config_filename = 'config'
server_host = 'irc.freenode.net'
server_port = 6667
channel_name = '#Channel_Name'
channel_key = 'key'
robot_name = 'Yibot'
robot_nick = 'yeast_robot'
PLAYING = False
player_nick = ''
left_times = 0
OP_PRI = {
    '^':3,
    '+':1,
    '-':1,
    '*':2,
    '/':2,
    '(':0
    }
sock = socket.socket( socket.AF_INET, socket.SOCK_STREAM )

def send_msg(msg):
  global sock
  sock.sendall( bytes(msg, 'utf-8') )

def send_channel_msg(msg):
  global channel_name
  send_msg( 'PRIVMSG %s :%s\r\n' % (channel_name, msg) )

def console_log(msg, local=True):
  prefix_str = '  > ' if local else '  --> '
  print(prefix_str + msg)

def setup_channel():
  global config_filename
  global channel_name
  global channel_key

  # open config file
  config_file = open(config_filename)

  for line in config_file:
    # trim whitespaces and linebreaks of each line
    trimmed = line.strip()
  
    # get channel name in config
    matched = re.match(r"^CHAN='#(.*?)'$", trimmed)
    if matched:
      channel_name = '#' + matched.group(1)

    # get channel key in config
    matched = re.match(r"^CHAN_KEY='(.*?)'$", trimmed)
    if matched:
      channel_key = matched.group(1)

def calc_expression(exp):
  print('exp =', exp)

  chunks = []

  last_is_digit = False
  for character in exp:
    if character in '0123456789.':
      if last_is_digit:
        chunks[-1] += character

      else:
        chunks.append(character)
        last_is_digit = True
    
    elif character in '+-/*()^':
      chunks.append(character)
      last_is_digit = False
    
    else:
      last_is_digit = False
  
  print('chunks =', chunks)

  op_stack = []
  fp_stack = []
  for it in chunks:
    if it in '(^':
      op_stack.append(it)
    
    elif it in '+-*/':
      if len(op_stack) > 0:
        prev_op = op_stack[-1]
        curr_op = it
        while OP_PRI[prev_op] >= OP_PRI[curr_op]:
          if len(fp_stack) < 2:
            console_log('Cannot pop fp_stack')
            return (False, 0)
          
          fp1 = fp_stack.pop()
          fp2 = fp_stack.pop()
          if prev_op == '+':
            fp_stack.append(fp2 + fp1)
          elif prev_op == '-':
            fp_stack.append(fp2 - fp1)
          elif prev_op == '*':
            fp_stack.append(fp2 * fp1)
          elif prev_op == '/':
            fp_stack.append(fp2 / fp1)
          elif prev_op == '^':
            fp_stack.append(fp2 ** fp1)
        
          print('op_stack =', op_stack)
          print('fp_stack =', fp_stack)
          print()
          op_stack.pop()
          if len(op_stack) > 0:
            prev_op = op_stack[-1]
          else:
            break
      op_stack.append(it)

    elif it == ')':
      prev_op = op_stack[-1]
      while prev_op != '(':
        if len(fp_stack) < 2:
          console_log('Cannot pop fp_stack')
          return (False, 0)
        
        fp1 = fp_stack.pop()
        fp2 = fp_stack.pop()
        if prev_op == '+':
          fp_stack.append(fp2 + fp1)
        elif prev_op == '-':
          fp_stack.append(fp2 - fp1)
        elif prev_op == '*':
          fp_stack.append(fp2 * fp1)
        elif prev_op == '/':
          fp_stack.append(fp2 / fp1)
        elif prev_op == '^':
          fp_stack.append(fp2 ** fp1)
        
        print('op_stack =', op_stack)
        print('fp_stack =', fp_stack)
        print()

        op_stack.pop()
        prev_op = op_stack[-1]

      op_stack.pop()

    else:
      try:
        opd = float(it)
        fp_stack.append(opd)
        print('op_stack =', op_stack)
        print('fp_stack =', fp_stack)
        print()
      except ValueError:
        console_log('Cannot convert float')
        return (False, 0)
  
  while len(op_stack) > 0:
    prev_op = op_stack[-1]
    if len(fp_stack) < 2:
      console_log('Cannot pop fp_stack')
      return (False, 0)
    
    fp1 = fp_stack.pop()
    fp2 = fp_stack.pop()
    if prev_op == '+':
      fp_stack.append(fp2 + fp1)
    elif prev_op == '-':
      fp_stack.append(fp2 - fp1)
    elif prev_op == '*':
      fp_stack.append(fp2 * fp1)
    elif prev_op == '/':
      fp_stack.append(fp2 / fp1)
    elif prev_op == '^':
      fp_stack.append(fp2 ** fp1)
    
    print('op_stack =', op_stack)
    print('fp_stack =', fp_stack)
    print()

    op_stack.pop()

  if len(op_stack) != 0:
    return (False, 0)
  if len(fp_stack) != 1:
    return (False, 0)

  return (True, fp_stack[0])

print('\n********** robot.py **********\n')

console_log('Connecting to server ' + server_host)

try:
  sock.connect((server_host, server_port))
except socket.error as err_msg:
  console_log('Socket error: %s' % err_msg)
except Exception as err_msg:
  console_log('Caught exception: %s' % err_msg)

if sock:
  console_log('Connect successfully')
else:
  console_log('!!! no built socket')
  exit(1)

console_log('Sending USER for authentication')
send_msg('USER ' + robot_name + ' ' + robot_name + ' ' + robot_name + ': Hi.\r\n')

console_log('Setting up robot nickname')
send_msg('NICK ' + robot_nick + '\r\n')

console_log('From config file, setting up channel info')
setup_channel()

console_log('Joining the channel ' + channel_name)
send_msg('JOIN ' + channel_name + ' ' + channel_key + '\r\n')

send_channel_msg('Hi, I\'m %s' % robot_nick)

while True:
  data = sock.recv(4096)
  
  if data:
    recv_msg = data.decode('utf-8').strip('\r\n')
    console_log(recv_msg, local=False)

    # server is checking the robot
    matched = re.match(r'^PING :', recv_msg)
    if matched is not None:
      ping_val = recv_msg[6:]
      reply_msg = 'PONG :%s\r\n' % ping_val
      send_msg(reply_msg)
      console_log('Received PING, reply with %s' % reply_msg)

    # someone is speaking
    matched = re.match(r'^:(.+)!(.+) PRIVMSG #(\w+) :(.+)$', recv_msg)
    if matched is not None:
      talker_nick = matched.group(1)
      message = matched.group(4)

      match_help = re.match(r'^@help$', message)
      if match_help is not None:
        reply_msg = '@repeat <String> (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        reply_msg = '@play <Robot Name> (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        reply_msg = '@guess <Integer> (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        reply_msg = '@cal <Expression> (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        reply_msg = '@weather <Taiwan City Name in TC> (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        console_log('#%s request @help' % talker_nick)

      match_repeat = re.match(r'^@repeat (.*)$', message)
      if match_repeat is not None:
        reply_msg = match_repeat.group(1) + ' (%s)\r\n' % talker_nick
        send_channel_msg(reply_msg)
        console_log('#%s request @repeat' % talker_nick)

      match_play = re.match(r'^@play ' + robot_nick + '$', message)
      if match_play is not None:
        if PLAYING:
          if talker_nick == player_nick:
            reply_msg = 'I\'m playing with you(%d times left) (%s)\r\n' % (left_times, talker_nick)
          else:
            reply_msg = 'Sorry~ I\'m playing with others... (%s)\r\n' % talker_nick
          send_channel_msg(reply_msg)
        else:
          reply_msg = 'Yeah~ Let\'s play(From 0-100, 5 times left)! (%s)\r\n' % talker_nick
          send_channel_msg(reply_msg)
          PLAYING = True
          player_nick = talker_nick
          left_times = 5
          answer_val = randint(0,100)
        console_log('#%s request @play' % talker_nick)

      if PLAYING and talker_nick == player_nick:
        match_guess = re.match(r'^@guess ([0-9]{1,3})', message)
        if match_guess is not None:
          guess_val = int(match_guess.group(1))
          left_times -= 1
          
          if guess_val > answer_val:
            reply_msg = 'Lower! (%d times left) (%s)\r\n' % (left_times, talker_nick)
          elif guess_val == answer_val:
            reply_msg = 'CORRECT! (%s)\r\n' % talker_nick
            PLAYING = False
            player_nick = ''
            left_times = 0
          else:
            reply_msg = 'Higher! (%d times left) (%s)\r\n' % (left_times, talker_nick)
          
          if PLAYING and left_times == 0:
            reply_msg = 'GAMEOVER~~~ The answer is %d! (%s)\r\n' % (answer_val, talker_nick)
            PLAYING = False
            player_nick = ''
            left_times = 0

          send_channel_msg(reply_msg)
          console_log('#%s request @guess\r\n' % talker_nick)

      match_cal = re.match(r'^@cal (.+)', message)
      if match_cal is not None:
        expression = match_cal.group(1)
        check_char = re.match(r'^[0-9.+\-*/^\s()]*$', expression)
        
        if check_char is not None:
          check_operand = re.search(r'[0-9.]\s+[0-9.]', expression)
          check_operator = re.search(r'[+\-*/^]\s+[+\-*/^]', expression)
          check_start = re.match(r'\s*[+\-*/^]', expression)
          
          if check_operand is None and check_operator is None and check_start is None:
            try:
              runnable, result = calc_expression(expression)
            except Exception:
              runnable = False
          else:
            runnable = False

          if runnable:
            reply_msg = '%s (%s)\r\n' % (str(result), talker_nick)
          else:
            reply_msg = 'Syntax error (%s)\r\n' % talker_nick
        
        else:
          reply_msg = 'Syntax error (%s)\r\n' % talker_nick
        
        send_channel_msg(reply_msg)
        console_log('#%s request @cal\r\n' % talker_nick)

      match_weather = re.match(r'^@weather (.*?)', message)
      if match_weather is not None:
        req = requests.get('http://www.cwb.gov.tw/V7/forecast/week/week.htm')
        req.encoding = 'utf-8'
        soup = BeautifulSoup(req.text, 'html.parser')
        bigtable = soup.find('table', class_='BoxTableInside')

        city_name = message[9:]
        tablerows = bigtable.find_all('tr')
        titlerow = tablerows[0]

        # City specific search
        citytag = bigtable.find('th', text=city_name)
        if citytag:
          dayrow = citytag.parent
          nightrow = dayrow.findNext('tr')

          titletext = ['City']
          for th in titlerow.find_all('th'):
            if th.text:
              titletext.append(th.text)

          daytext = []
          daytext.append( dayrow.find('th').text )
          daytext.append( dayrow.find('td').text )
          for td in dayrow.find_all('td')[1:]:
            daytext.append( '%s (%s)' % (td.find('img')['title'],td.text.strip()) )

          nighttext = []
          nighttext.append( daytext[0] )
          nighttext.append( nightrow.find('td').text )
          for td in nightrow.find_all('td')[1:]:
            nighttext.append( '%s (%s)' % (td.find('img')['title'],td.text.strip()) )

          for i in range(2, 5):
            weather_info = '%s %s => %s: %s, %s: %s (%s)\r\n' % \
              (daytext[0],titletext[i],daytext[1],daytext[i],nighttext[1],nighttext[i],talker_nick)
            send_channel_msg(weather_info)
        
        else:
          send_channel_msg('City name not found or syntax error (%s)\r\n' % talker_nick)
        
        console_log('#%s request @weather %s\r\n' % (talker_nick, city_name))

sock.close()

