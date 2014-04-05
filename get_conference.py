#!/usr/bin/env python
# -*- coding: utf-8 -*-
from BeautifulSoup import BeautifulSoup
import urllib2
import MySQLdb, string
db=MySQLdb.connect(passwd="glas982156",db="podcasts", user="root")
c=db.cursor()

url_base = 'https://www.lds.org/general-conference/sessions/2014/04?lang=eng'
mp3_addresses = []

def sanitize(text):
  text = text.strip()
  replacements = {
    'â' : '',
    '€' : '',
    'œ' : '',
    'Â' : '',
    'Â' : '',
  }
  for k, v in replacements.iteritems():
    #print 'replacing %s with %s' % (k, v)
    text = text.replace(k, v)
    pass
  return db.escape_string(text)

if __name__ == '__main__':
  page = urllib2.urlopen('%s' % (url_base))
  soup = BeautifulSoup(page)
  tables = soup.findAll('table');
  page_text = '%s' % soup
  conference = page_text.split('<title>')[1].split('</title>')[0]
  conference = string.replace(conference, "\n", '')
  conference = string.replace(conference, '  ', '')
  conference = string.replace(conference, '  ', '')
  conference = string.replace(conference, '  ', '')
  conference = string.replace(conference, '  ', '')
  conference = string.replace(conference, '  ', '')
  conference = string.replace(conference, '  ', '')


  #trs = tables.findAll('tr')
  trs = soup.findAll('tr')
  session = ''
  for tr in trs:
    text = '%s' % (tr)
    if 'head-row' in text:
      session = text.split('<h2>')[1].split('</h2>')[0]
      if 'session' in session:
        session = session.split('<session')[1].split('</session>')[0].split('>')[1]
        print 'session: %s' % (session)
    if '.mp3' in text and 'class="speaker"' in text and 'head-row' not in text:
      speaker = text.split('<span class="speaker">')[1].split('<')[0]
      title = ''
      if '<span class="talk">' in text:
        title  = text.split('<span class="talk">')[1].split('>')[1].split('<')[0]
      anchors = tr.findAll('a')
      for anchor in anchors:
        anchor_text = '%s' % anchor
        #print 'anchor_text : %s' % anchor_text
        if '.mp3' in anchor_text:
          address = anchor_text.split('href="')[1].split('"')[0]
          talk_id = '-'.join(address.split('/')[-1].split('-')[0:3])
      print 'address   : %s' % address
      print 'talk_id   : %s' % talk_id
      print 'session   : %s' % session
      print 'conference: %s' % conference
      print 'title     : %s' % title
      print 'speaker   : %s' % speaker
      sql = "SELECT speaker, last_played FROM conference WHERE talk_id='%s'" % talk_id
      print 'sql: %s' % sql
      print 'testing'
      c.execute(sql)
      response = c.fetchone()
      #for res in response:
        #print 'res: ',
        #print res
       # pass

      sql = """
      INSERT INTO conference SET talk_id='%s', session='%s', conference='%s', title='%s', speaker='%s', url='%s'
      ON DUPLICATE KEY UPDATE session='%s', conference='%s', title='%s', speaker='%s', url='%s'""" % (sanitize(talk_id), sanitize(session), sanitize(conference), sanitize(title), sanitize(speaker), sanitize(address), sanitize(session), sanitize(conference), sanitize(title), sanitize(speaker), sanitize(address) )
      print 'sql: %s' % sql
      c.execute(sql)
