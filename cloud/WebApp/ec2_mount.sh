#!/bin/bash
/usr/local/bin/s3fs sas5-unei-test-zybo  /mnt/zybo -o rw,allow_other,uid=1000,gid=1000,default_acl=public-read,iam_role="sas5_ec2_s3_mount_role"
/usr/local/bin/s3fs sas5-unei-test-face  /mnt/face -o rw,allow_other,uid=1000,gid=1000,default_acl=public-read,iam_role="sas5_ec2_s3_mount_role"
