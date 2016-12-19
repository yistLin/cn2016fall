#-*- encoding: utf-8 -*-
import requests
from bs4 import BeautifulSoup
import re

req = requests.get('http://www.cwb.gov.tw/V7/forecast/week/week.htm')
req.encoding = 'utf-8'
html_doc = req.text
# print(html_doc)

soup = BeautifulSoup(html_doc, 'html.parser')
bigtable = soup.find('table', class_='BoxTableInside')

tablerows = bigtable.find_all('tr')
titlerow = tablerows[0]
dayrow = tablerows[3]
nightrow = tablerows[4]

# City specific search
dayrow = bigtable.find('th', text="新北市").parent
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

for i in range(2,9):
  weather_info = '%s %s => %s: %s, %s: %s' % \
    (daytext[0],titletext[i],daytext[1],daytext[i],nighttext[1],nighttext[i])
  print(weather_info)
