#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import logging

logging.basicConfig(level=logging.DEBUG, 
    filename="",
    datefmt='%Y/%m/%d %H:%M:%S',
    format='%(asctime)s %(name)s %(levelname)s %(message)s')
 
def get_logger(logname):
    logger = logging.getLogger(logname)
    return logger
