$ *    SAM-QFS_notice_begin
$ *
$ * CDDL HEADER START
$ *
$ * The contents of this file are subject to the terms of the
$ * Common Development and Distribution License (the "License").
$ * You may not use this file except in compliance with the License.
$ *
$ * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
$ * or http://www.opensolaris.org/os/licensing.
$ * See the License for the specific language governing permissions
$ * and limitations under the License.
$ *
$ * When distributing Covered Code, include this CDDL HEADER in each
$ * file and include the License file at pkg/OPENSOLARIS.LICENSE.
$ * If applicable, add the following below this CDDL HEADER, with the
$ * fields enclosed by brackets "[]" replaced with your own identifying
$ * information: Portions Copyright [yyyy] [name of copyright owner]
$ *
$ * CDDL HEADER END
$ *
$ * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
$ * Use is subject to license terms.
$ *
$ *    SAM-QFS_notice_end
1000 Gen
1001 INQUIRE command failed
1002 Device is %s, %s
1003 Serial %s, rev %s
1004 Rev %s
1005 Known as %s(%s)
1006 Slot %d
1007 Checking ready
1008 Changing state to DOWN
1009 Changing state to OFF
1010 SCSI error: %s
1011 Unknown device:  Update /etc/opt/SUNWsamfs/inquiry.conf (see inquiry.conf(4))
1012 open(%s) failed errno = %d
1013 Slot %d: Ready time %d seconds
1014 Wait becoming ready
1015 Delaying %d seconds for robot
1016 Device ready/spun down/unloaded
1017 Attempt open(%s) %#x
1018 fstat(%s)
1019 Opened fd = %d, path = %s, open count = %d
1020 Close %d, open count = %d
1021 scsi_cmd(%#x) entered without io_mutex
1022 scsi_cmd(%#x) entered with io_active
1023 scsi_cmd: EAGAIN
1024 scsi_cmd(%#x)
1025 Read log failed
1026 Could not lock door
1027 Unloaded and door unlocked
1028 Loading tape libraries for f-set %d
1029 Archiver not responding
1030 Message %#x
1031 Not Migration Toolkit device
1032 Signal preview
1033 Unloaded and door unlocked
1034 Library device not present
1035 Not a library
1036 Attempt to label robotic device
1037 Attempt to label active device
1038 Attempt to label read only medium
1039 Medium already labeled
1040 open(%s)
1042 Device type mismatch mcf = %s, inq = %s
1043 Don't know about barcodes
1044 open(%s)
1045 Load: unknown device type %#x
1046 Bad callback in mount_thread
1047 Load VSN %s
1048 open(%s)
1049 open(%s)
1050 Open %s as %d, open count = %d
1051 close(%d)
1052 close(%d)
1053 Close %d, open count = %d
1054 Device has no active_io struct
1055 Already waiting/active
1056 File system call failed (FSMOUNT)
1057 Load request canceled
1058 Load error
1059 Ready for data transfer
1060 SC_fsiocount failed
1061 %#x blocks transferred
1062 Waiting for response to timeout
1063 SC_fscancel enter: errno %d, oldio %#x, newio %#x
1064 SC_fscancel
1065 Direct I/O timed out
1066 Wrote %lld bytes, time %d seconds
1067 Io_count: diff %d, count %d
1068 Io_count from %#x to %#x final %#x
1069 Read %lld bytes, time %d seconds
1070 update_catalog_space: entry not set
1071 Remote sam not found
1072 Unable to open for stage
1073 syscall(SC_fsstage)
1074 Unable to start close thread
1075 close() after stage time: %d seconds
1076 ** NOT USED ** Stage error reading removable media from slot %d: error %#x
1077 Read error
1078 Stage error, Checksum miscompare
1079 ioctl(F_SWRITE) inode %ld
1080 Swrite returned 0, cancel stage
1081 Error on inode(%d.%d) : Error %d
1082 Staged %lld bytes, time %d seconds
1083 Start stage file thread
1084 Start stage file thread
1085 Start stage file thread
1086 mount_remote_stage failed, cleanup and exit
1087 Stage using %d buffers of %#x bytes
1088 Helper tid not zero: old %d, new %d
1089 Attempt to stage inode %d from re-labeled medium
1090 Stage request for %d:%d.%d canceled by fs
1091 Sent RMT_SAM_DONE
1092 io tid != 0
1093 Stage File Reader Start
1094 PDU size %#llx is not a power of two
1095 Buffer size %d is not a multiple of sector size
1096 Next read : Data left %#llx PDU size %#llx  req len %#llx  pos %#x  buf idx %d  size adjust %#llx
1097 Recoverable error
1098 Staging error(key=%#x): premature EOF on read (%#llx of %#llx bytes not read for this file)(pda-%#llx)
1099 Blank check on read
1100 Read error: medium error
1101 Stage Reader exit
1102 Unknown media type %s
1103 Head of stage list-NULL
1104 Stage position == 0
1105 Stage READER (inode %d.%d) POS %llx: archive header is not valid
1106 Helper shutdown
1107 Helper tid %d != %d
1108 Shutdown stage reader
1109 Append stage list (%#x, %#x) (%#x, %#x)
1110 Add at the end (%#x, %#x) (%#x, %#x)
1111 Staged Write Top : size %#llx, file_size %#llx
1112 Cancel detected in request
1113 Stage Writer Request Start : Buffer(%d):s:%#x, e:%#x, p:%#x, l:%#x, o:%#x, ds:%#x
1114 sw o:%#llx, i:%#x, s:%#x
1115 Cancel detected while writing inode %ld
1116 Cancel detected in request inode %ld/
1117 close(%d)
1118 Changing state to ON
1119 Spin unit [%d] containing slot %d fails: error %d
1120 Start Stage : File size %#llx adjustments %#llx %#llx pdu %#llx
1121 Read Complete : %d bytes for request of %d
1122 Adjust buffer %#llx buffer size %d pdu %#llx req len %#llx
1123 New position %x adjust %#llx
1124 Reader Pass done : Total %#llx pda %#llx offset %#llx size %d adjust %#llx
1125 Reader Start Pass : data left %#llx
1126 Read Complete (<=0) : %d bytes for request of %d
1127 Recoverable : key %#x now %#x next %#x
1128 Log error %#x
1129 Log error %#x
1130 Stage Writer Complete for %d:%d.%d 
1131 Calculate checksum
1132 Log error %#x
1133 VSN %s: Stage error: Checksum miscompare:%s
1134 Ino:CPOS:%#llx PDU:%#llx IPOS:%#x IFO:%x VSN:%s CP:%d
1135 couldn't stat %s, inode %#x, errno: %d
1136 could not open %s, errno: %d
1137 could not find mount point eq %d
1138 Stage WRITER (inode %d.%d) POS %llx: archive header is not valid
1139 Stage READER (inode %d.%d): cur:%llx, expected %llx != tape %x
1140 Stage READER (inode %d.%d): Rewind, reposition, and retry stage
1141 %08x  %s
1142 Stage READER (inode %d.%d) POS %llx: archive header not at end of file
1143 Stage READER (inode %d.%d): Reposition, and retry stage
1144 Stage Position: %02d: %08x
1145 Locate Position to %llx (inode %d.%d); Start = %llx; Tape position is %x
1146 %d:tr=%d,POS:%llx data_left:%#llx pdu:%#llx req_len:%#llx bi:%d sz_adj:%#llx
1147 %d:Buf%d%cv:%d sz:%x rsz:%x dat_sz:%x off:%x pos:%x lpda:%x err:%d
1148 Read Position error %d
1149 Tape not at expected position %llx, at %x. Position tape before first read.
1150 Slot %d switched to write-protected since last mounted.
1151 Failed to open the path %s
1152 Opened in read-write mode: fd = %d, path = %s, open count = %d
1153 Opened in read-only mode: fd = %d, path = %s, open count = %d
1154 Perform Test Unit Ready (TUR): Slot %d: Starting TUR
1155 Ready time %d seconds
1156 Spin unit [%d] fails: error %d
1157 Perform Test Unit Ready (TUR): Starting TUR
1158 Stage error reading removable media error %#x
1159 Std inquiry adr16 %d, multip %d, wbus16 %d, sync %d, clk %d
1160 WWN %s, type %d
1161 Port id %d
1162 Lun protocol %x (%s)
1163 Port protocol %x (%s)

2000 Optic
2001 Erase started
2002 Erase failed
2003 Write second anchor failed
2004 Writing anchor %d at %#lx
2005 Write anchor failed
2006 Read anchor %d at %#lx
2007 Read anchor %d failed
2008 Write volume descriptor block %d at %#lx
2009 Write volume descriptor block %d failed
2010 Write volume label %d at %#lx
2011 Write volume label %d failed
2012 Determine capacity
2013 Read capacity - retry %d
2014 Read capacity failed
2015 Mode sense failed
2016 Medium type 0 returned
2017 Invalid medium
2018 %d byte sector not supported
2019 Medium type %#x, capacity %lld, sector size %d, write protect %d
2020 Read anchor sector - retry %d
2021 Read label failed
2022 Setting range %#llx-%#llx
2023 Read ANSI label failed
2024 Label %s %s
2025 Read PTOC medium error - retry %d
2026 ptoc fwa invalid %#x, should be 0x100 < loc < %#x
2027 Read PTOC - retry %d
2028 Search for PTOC
2029 Label not found
2030 Scanning read failed
2031 Locating lower bound failed
2032 Searching for PTOC failed
2033 Find first label sector failed
2034 Read eof sector failed
2035 close(%d)
2036 Label verify error
2037 Unable to read old labels
2038 Old label not ANSI
2039 Unable to label
2040 Medium error
2041 Device error
2042 Missing EOF: %s, %s
2043 Unable to write cur_ptoc_fwa
2044 Label scan failed
2045 Spinup time %d seconds
2046 Unload time %d seconds
2047 Begin write label, VSN %s
2048 Error reading old labels
2049 ioctl(SAMSTIOC_RANGE) failed
2050 Error during label scan
2051 ***
2052 ***
2053 ***
2054 ***
2055 Setting range %#llx-%#llx
2056 ioctl(%s, SAMSTIOC_RANGE) failed
2060 Read volume label %d at %#lx
2061 Read volume label %d failed
2063 Write PTOC value %#lx
2064 Clear PTOC failed
2065 Read PTOC at %#lx
2066 Read PTOC failed
2067 Read PTOC value %#lx
2068 PTOC fwa %#lx less than lwa %#lx
2069 PTOC lwa value %#lx
2070 Write optical EOF at %#lx
2071 Write optical EOF failed
2072 Read optical EOF at %#lx
2073 Optical EOF found
2074 Read optical BOF at %#lx
2075 Optical BOF found
2076 Space remaining %ld, Capacity %ld
2077 Optical BOF corrupted
2080 Setting optical driver block size to %d
2081 ioctl(SAMSTIOC_SETBLK) failed
2082 Write optical BOF at %#lx
2083 Write optical BOF failed
2084 Read BOF/EOF data at %#lx for %d bytes
2085 EOF detection: EO found
2086 Sector size %d, label size %d
2087 Write optical BOF/EOF at %#lx
2088 Write optical BOF/EOF failed
2090 Next file FWA %#lx
2091 Setting file start position to %#lx
2092 Next file FWA %#lx larger than PTOC %#lx
2093 EOF not found, skipping unclosed file entry
2094 Using archive label PDA for PTOC
2095 Partial read error, requested %d, received %d
2096 Short read on sector data (sectors read %d, needed at least %d)
2097 Relabeling media
2098 Relabeling WORM media not allowed
2099 Cleaning up unclosed file
2100 Verifying optical media starting at %#lx for %d sectors
2101 Unable to update catalog for relabel: %s
2104 Inquiry device id parse %d %d %d


3000 Tape
3001 mt_erreg = %#x [%s]
3002 device status is SUN_KEY_EOT(%#x)
3003 Label %s %s blocksize = %d
3004 Starting erase
3005 Write labels failed, count = %d
3006 Unable to get status: Old VSN %s, New VSN %s
3007 Status: dsreg %#x, erreg %#x, resid %#x, fileno %#x, blkno %#x, flags %#x
3008 re-open(%s)
3009 Unable to label
3010 Unable to rewind
3011 Unable to label: Old VSN %s, New VSN %s
3012 Unable to rewind
3013 Unable to write file
3014 Unable to rewind
3015 Write label - rewinding
3016 Unable to rewind: Old VSN %s, New VSN %s
3017 Unable to erase
3018 Write label - rewinding
3019 Unable to rewind: Old VSN %s, New VSN %s
3020 re-open(%s): Old VSN %s, New VSN %s
3021 Writing labels
3022 Writing labels - vol1
3023 Writing labels - hdr1
3024 Writing labels - hdr2
3025 Unable to rewind
3026 open(%s)
3027 Unknown device type %#x
3028 Slot %d: Recovery needed
3029 Slot %d: Recovery failed
3030 Slot %d: Recovery time %d seconds
3031 Rewind time %d seconds
3032 Drive containing slot %d did not become ready
3033 open(%s)
3034 Setting compression
3035 Compression set
3036 Compression not set: %s failed
3037 Skip to EOD failed, trying recovery
3038 Possible bus reset
3039 open(%s) failed, retry %d
3040 Unable to open (%d) or skip to EOM (%d)
3041 Unable to read position
3042 Positioned to %#x after reset
3043 Impossible position - %#x
3044 Bad medium
3045 Nothing written on tape
3046 Writing EO%c at %#x
3047 Unexpected tape position (%d-0x%x), labels will not be written
3048 EOT detected, rewinding
3049 Unable to create EOF at %#x
3050 Skip-eod-F time: %d seconds
3051 Rewind time: %d seconds
3052 Skip-eod-F time: %d seconds
3053 Unable to position to EOD
3054 Skip to EOD time: %d seconds
3055 Skip to EOD time: %d seconds
3056 EOD at position %#x
3057 No space left on tape
3058 Position error: wanted %#x, got %#x
3059 append_tape: Found EOF1 at %#x
3060 append_tape: Found EOV1 at %#x
3061 Medium error: No space left
3062 append_tape: EOF1 not found at %#x [%d, %d, %d, %#x, %#x, %#x, %#x, %#x]
3063 append_tape: Unable to position to %#x
3064 BSR failed
3065 BSF failed
3066 Positioning time: %d seconds
3067 Positioning error to %#x
3068 Position from %#x to %#x: %d seconds
3069 Begin write label, VSN %s
3070 Find EOD - skip to EOD
3071 Skip to EOD time: %d seconds
3072 Rewind time: %d seconds
3073 EOD position bad, searching for EOD
3074 EOD unknown, searching for EOD
3075 Skip to EOD time: %d seconds
3076 Unable to position to EOD
3077 Capacity at EOD returned as 0
3078 Finding EOD - backspace at EOD
3079 Find EOD-No tape mark at EOD
3080 Position at EOD is %#x
3081 Unable write tape mark at EOD [status %d errno %d]
3082 Finding EOD - BSF at EOD
3083 Did not become ready: Error %d, Old VSN %s, New VSN %s
3084 Waiting for ready: retrys %d
3085 Append pos %#x, curr %#x
3086 New current %#x
3087 Position error: wanted %#x, got %#x
3088 Rewind failed
3089 Recover from position error: Positioning to %#x
3090 Position failed: to %#x
3091 Position time: %d seconds
3092 Position failed
3093 Positioned to %#x
3094 Skip backwards at image start
3095 Position with FSR %d
3096 Problems skipping records (%#x,%#x,%#x:%c%c)
3097 Rewinding
3098 Skip-%c %#x: %d seconds
3099 Unable to determine capacity
3100 Sense EOM
3101 Sense EOM
3102 Drive reported an invalid capacity (%#llx).  Capacity reset to default.
3103 Medium capacity is 0
3104 Unable to determine capacity
3105 Unable to determine capacity: invalid data'
3106 Medium capacity %#llx larger than UINT_MAX
3107 Unable to determine capacity (Log Sense command failed)
3108 Medium capacity %#llx larger than UINT_MAX
3109 Unable to determine capacity
3110 Capacity FM(%d), POS(%d), CAP(%d), USE(%d)
3111 Unable to determine capacity
3112 Unknown media %#x
3113 Unable to determine capacity
3114 Medium capacity %#llx larger than UINT_MAX
3115 Sense EOM
3116 Unable to determine capacity: does not support function
3117 Device is type default. Update /kernel/drv/st.conf
3118 Mode sense failed
3119 Medium is write protected
3120 Process labels: rewinding
3121 Unable to rewind
3122 Rewind time %d seconds
3123 Process labels: Read labels
3124 READ_BLKLIM failed: setting blocksize to %d
3125 Maximum block size %d (returned %d)
3126 Loading library %s
3127 Mapping entry %s
3128 Error mapping symbol %s: %s
3129 Loading library %s failed
3130 Library %s loaded
3131 ** NOT USED ** Returning %s for D2 second device path %s
3132 Unable to get samst device for non-/dev/rmt/ device:%s
3133 lstat(%s)
3134 Tape path is not sym-link - %s
3135 readlink(%s)
3136 Unable to get samst name for non st device:%s
3137 Bad mt device name %s
3138 DST medium needs formatting
3139 Runtime interface not found
3140 Runtime interface not found
3141 Runtime interface not found
3142 Runtime interface not found
3143 Runtime interface not found
3144 Runtime interface not found
3145 Runtime interface not found
3146 Runtime interface not found
3147 Runtime interface not found
3148 Runtime interface not found
3149 Runtime interface not found
3150 Runtime interface not found
3151 Runtime interface not found
3152 BSR loop
3153 Unable to BSF-l during EOD search
3154 BSF loop
3155 Unable to FSF during EOD search
3156 Found ?-TM-EOD
3157 Position at EOF-TM-TM (%#x) is < supplied position (%#x)
3158 Unable to find EOD
3159 Found no eof at position %#x
3160 Unable to find EOF at position %#x
3161 Found Record-TM-EOD, skipping to EOD
3162 Unable to skip to EOD
3163 Position after TM (%#x) is < supplied position (%#x)
3164 Unable to find EOD
3165 Unable to find EOD at position %#x
3166 Unable to BSF-2 EOD search
3167 Unable to find EOD
3168 Found ?-TM-TM-EOD
3169 Position after TM-TM-TM (%#x) is < supplied position (%#x)
3170 Unable to find EOD
3171 Unable to find EOD at position %#x
3172 Found R-TM-TM-EOD during EOD search
3173 Unable to BSR-2 EOD search
3174 Position at EOF-TM-TM (%#x) is < supplied position (%#x)
3175 Unable to find EOD
3176 Found EOF1 at %#x
3177 Unable to find EOD at position %#x
3178 Found R-TM-TM-EOD, not EOF1
3179 Position after R-TM-TM-EOD (%#x) is < supplied position (%#x)
3180 Unable to find EOD
3183 Positioning to %#x
3184 Invalid resource record
3185 Unable to position tape to EOD
3186 Skip to EOD time: %d seconds
3187 Unable to position to EOD
3188 Cleaning bit detected
3189 Expecting SUN_KEY_EOF found %x
3190 Read position error at %#x
3191 Read position error at %#x
3192 Read position error at %#x
3193 Unable to determine position
3194 Waiting for tape to close
3195 Unable to BSF-1 during EOD search
3196 Rewind time: %d seconds
3197 Runtime interface not found
3198 Sense EOM
3199 Media Error : Slot %d : Cannot read label
3200 after erase failure, scsi reset failed
3201 after erase failure, scsi rewind failed
3202 Write of file mark fails - error %d, cleaning %d
3203 Check for End of Tape : eot 0x%x read_pos err 0x%x
3204 MTIOCGET ioctl fails : error %d
3205 resource ptr: 0x%x, process_eox: %d, process_wtm: %d, wrote_tm: %d
3206 End of tape detected while writing EOF
3207 On first attempt unable to write EO%c: errno %d [write return %d]
3208 End of tape detected on write of file mark - EOP %d, cleaning %d
3209 End of tape sensed
3210 Write of file mark fails - error %d, cleaning %d
3211 End of tape detected on write of file mark - EOP %d, cleaning %d
3212 Writing file mark
3213 Read capacity %lld : %d 0x%x,0x%x,0x%x,0x%x
3214 Read capacity type %d
3215 Read capacity %d, %d
3216 File marks %d exceed %d
3217 Tape position %x before read does not match expected %llx
3218 Tape position %x after read does not match expected %llx
3219 Slot %d: Not Ready [ sense %d, %d/%d ]
3220 Creating EOF/EOV label
3221 attempt to %s media - retries exhausted
3222 MTIOCGET returns %d dsreg %d erreg %d errno %d
3223 Back Space %d records : erreg %d resid %d
3224 Unable to find VOL1 at load point
3225 Repair EOF on tape
3226 Rewind and skip to EOM succeeds
3227 Repair EOM returns %d
3228 Second attempt to write EOF/EOV failed: errno %d [write return %d]
3229 Check EOT [key %d eom %d]
3230 Copy in for EOV processing [eot = %d]
3231 Drive has bad media [slot %d]
3232 Illegal request on load/unload
3233 Position timeout set to %d secs for %d
3234 Skip to EOM succeeded
3235 Skip to EOM fails
3236 Unloaded vsn: %s, slot: %d
3237 Write TM at %#x
3238 Write %d TMs at %#x
3239 Drive supports media as read-only, setting write protect 
3240 End of tape detected on flush write of file mark - EOP %d, cleaning %d
3241 Flush write of file mark fails - error %d, cleaning %d
3242 Flush write of file mark fails - error %d, cleaning %d
3243 End of tape detected on flush write of file mark - EOP %d, cleaning %d
3244 Read tape position %#x (old %#x)
3245 Blankcheck detected on label read
3246 Space: Calculated space(%dK), Reported position(%d), Default capacity(%dK)
3247 Space: Drive-reported space(%dK)
3248 Unloaded vsn %s
3249 Media Error : Cannot read label
3250 Recovery needed
3251 Recovery failed
3252 Recovery time %d seconds
3253 Drive did not become ready
3254 Not Ready [ sense %d, %d/%d ]
3255 Unable to determine if drive VolSafe capable
3256 Unable to determine if media is VolSafe
3257 Unable to determine if WORM or RW media is loaded
3258 WORM media not supported
3259 WORM media supported
3260 VolSafe media write protected
3261 VolSafe media supported
3262 WORM media loaded
3263 VolSafe media loaded
3264 Unable to relabel used VolSafe media
3265 Read position failed near %x
3266 Locate failed, current position %x requested position %x
3267 Issuing load command
3268 Drive is in CD-ROM emulation mode
3269 Writing file mark flushing buffer for EOF/EOV
3270 Back Space %d filemarks : erreg %d resid %d
3271 Forward Space %d filemarks : erreg %d resid %d
3272 EOF1 or EOV1 not found at %#x.
3273 Found EOF1/EOV1 at %#x.
3274 Append at position %#x.
3275 Forward Space %d records : erreg %d resid %d
3277 Cleaning required
3278 Cleaning requested 
3279 Cleaning media is expired
3280 Auto-clean with log sense is enabled
3281 Auto-clean is enabled
3282 Cleaning media is invalid for tape drive
3283 Clearing cleaning bit, drive is clean
3284 Drive %d is encryption capable
3285 Attempt to position while writing, position failed.
3286 Unable to write TM at %#x

4000 Common
4001 Close %d, open_count = %d
4002 No event - active_count = %d
4003 Close %d, open_count = %d
4004 Invalid drive thread event = %d
4005 Unknown load request %#x
4006 Cannot find %d:%s in catalog
4007 Requested is not set
4008 Slot %d:%d, vsn %s, internal_address %#x
4009 Slot %d:%s is empty
4010 get_media() returned %d for slot %d, VSN %s
4011 Setting device state to off
4012 Loaded %d:%s
4013 Scanned %d:%s
4014 Loaded (not sam) %d:%s
4015 Not same preview: %#x, %#x
4016 Writing label %d:%s barcode = %s
4017 Media Error : Slot %d : Medium with barcode %s not labeled
4018 Media Error : Slot %d : Medium not labeled
4019 open_unit(%s)
4020 open_unit(%s)
4021 Cannot get catalog entry for %d:%d:%d
4022 Scanning %d:%d:%d
4023 Media Error : Medium with barcode %s not labeled
4024 Media Error : Medium not labeled

5000 Generic
5001 Cannot clear drive: Can not remove slot %d from drive: move failed
5002 Robot %d full
5003 Mailbox address (%d) is valid
5004 Mailbox address (%d) is not valid
5005 Finding empty slot
5006 Library does not support barcodes
5007 Robot range error (%d, %d, %d, %d)
5008 Slot range error (%d, %d, %d, %d)
5009 Mailbox range error (%d, %d, %d, %d)
5010 Drive range error (%d, %d, %d, %d)
5011 Read transport element status failed
5012 Robot count is %d, expected %d
5013 Incorrect robot page code %#x
5014 Read storage element status failed
5015 Slot count is %d, expected %d
5016 Incorrect element address page code %#x
5017 Storage element exception(%#x:%#x)
5018 Read import/export element status failed
5019 Mailbox count is %d, expected %d
5020 Incorrect mailbox page code %#x
5021 Read data transfer element status failed
5022 Drive count is %d, expected %d
5023 Incorrect drive page code %#x
5024 Robot exception bit set (%#x:%#x)
5025 Robot exception cleared
5026 Robot mismatch is %d, expected %d
5027 Mailbox exception bit set (%#x:%#x)
5028 Mailbox exception cleared
5029 Mailbox mismatch is %d, expected %d
5030 Drive exception bit set (%#x:%#x)
5031 Drive exception cleared
5032 Drive mismatch is %d, expected %d
5033 No scsi error table loaded for %s
5035 fstat(%s)
5036 Catalog size inconsistent: st_size %d, tablen %d, catent %d, cata %d
5037 Exception bit set, force init_element
5038 InitElement - retry %d
5039 Drives = %d
5040 Reinitialize library - Unload drives
5041 Library ready
5042 ** NOT USED ** Found labeled cartridge %s for slot %d
5043 thr_create(unload_a_drive)
5044 Mode select
5045 Mode select failed
5046 Unable to unload
5047 Initializing
5048 exchange_cartridge: from %d(%s) to %d to %d(%s)
5049 Move cartridge: from %d(%s) to %d
5050 Flip and scan
5051 Slot %d cartridge loaded
5052 Load cartridge from slot %d
5053 Slot %d not occupied
5054 Load %s failed, error %d
5055 Wait for idle
5056 Exchange media: from %d(%s) to %d to %d(%s)
5057 Move cartridge: from %d(%s) to %d
5058 thr_setconcurrency()
5059 Read transport element status failed
5060 Robot count is %d, expected %d
5061 Incorrect robot page code %#x
5062 Read storage element status failed
5063 Slot count is %d, expected %d
5064 Incorrect slot page code %#x
5065 Storage element exception(%#x:%#x)
5066 Read import/export element status failed
5067 Incorrect mailbox page code %#x
5068 Read data transfer element status failed
5069 Incorrect drive page code %#x
5070 Mode sense would overflow
5071 Read element status failed
5072 Unloading drives for export
5073 status_element_range() failed
5074 Finding EOD on %s
5075 Needs cleaning
5076 ** NOT USED ** No cleaning cartridge in slot %d
5077 Attempt to unload cleaning cartridge
5078 Unload of cleaning cartridge - retry %d
5079 Unload of cleaning cartridge
5080 Unload of cleaning cartridge failed
5081 thr_kill(%d)
5082 Event(s) lost
5083 Library active %d, first %#x
5084 Exchange_media recovered
5085 Rotate mailbox
5086 Init element status
5087 No page zero for mode select
5088 Invalid slot source %d dest %d
5089 Skip off/down drive for audit
5090 ** NOT USED ** Slot %d is out of range
5091 Slot %d is not in use
5092 Slot %d does not contain a SAM-FS cartridge
5093 Remote sam not found
5094 Media changer does not support move
5095 ** NOT USED ** Invalid slot source %d dest %d
5096 Source and destination are the same %d
5097 Source slot %d is not occupied
5098 Destination slot %d is occupied
5099 Remote sam not found
5100 No mailboxes found
5101 ** NOT USED ** Imported cartridge for slot %d has no barcode
5102 Imported cartridge for slot %d has barcode %s
5103 ** NOT USED ** Imported cartridge has a duplicate barcode (%s) with slot %d - exporting it
5104 ** NOT USED ** Imported cartridge has a duplicate barcode (%s) with slot %d - exporting it
5105 ** NOT USED ** Imported cartridge duplicates VSN %s in slot %d - exporting it
5106 ** NOT USED ** Imported cartridge duplicates VSN %s in slot %d - exporting it
5107 Remote sam not found
5108 Library full
5109 Import failed, check mailbox
5110 Imported cartridge has no barcode
5111 ** NOT USED ** Imported cartridge has a duplicate barcode (%s) with slot %d - exporting it
5112 ** NOT USED ** Imported cartridge duplicates VSN %s in slot %d - exporting it
5113 Remote sam not found
5114 Unable to lock mailbox door
5115 Mode sense page(s) 1d, 1f or 1e missing
5116 thr_create(drive_thread)
5117 thr_create(transport_thread)
5118 thr_create(import_thread)
5119 Catalog name not set
5120 open(%s)
5121 ftruncate(%s)
5122 mmap(%s)
5123 Unknown library type %#x
5124 Mailbox count is %d, expected %d
5125 Drive count is %d, expected %d
5126 Cannot clear drive: Can not remove slot %d from drive: move failed
5127 Cannot clear mailbox %d: move failed
5128 Cannot clear mailbox %d: move failed
5129 Test unit ready failed
5130 Mechanical hindrance
5131 Must be placed in random mode
5132 Initialize element failed
5133 Sense Key %d, ASC %d, ASCQ %d, SCSI error: %s
5134 Cannot clear drive: library full
5135 Cannot clear robot %d: library full
5136 Cannot clear mailbox %d: library full
5137 Mailbox %d cleared
5138 Mailbox %d cleared
5139 ** NOT USED ** Duplicate label %s in slot %d and %d - exporting element address %d
5140 ** NOT USED ** Duplicate label %s in slot %d and %d - exporting element address %d
5141 No cleaning cartridge available
5142 No usable cleaning cartridge available
5143 Unable to load cleaning cartridge: move failed
5144 Cleaning cartridge in slot %d has expired
5145 Cleaning cartridge in slot %d has expired - exporting
5146 Unable to unload cleaning cartridge: move failed
5147 Unable to unload cleaning cartridge
5148 Import thread request bad %#x
5149 Import thread request bad magic
5150 Import thread request bad command %#x
5151 Import thread request bad event %#x
5152 Unable to clear robot
5153 Mailbox not supported
5154 Import failed: library full
5155 Cannot audit to EOD on non-SAM cartridge
5156 Audit to EOD on imported cartridge
5157 ***
5158 Mailbox not supported
5159 No empty mailbox
5160 Mailbox full
5161 Unable to unload drive: move failed
5162 Move media failed: library in unknown state
5163 Remote sam not found
5164 Remote sam not found
5165 Export: VSN not found
5166 Export: slot number %d out of range
5167 Export: drive %d not found
5168 Export: drive %d is empty
5169 Export: bad request %#x
5170 Export: slot %d not occupied or unavailable
5171 Export: unable to find cartridge
5172 Export: cartridge is in a drive and the drive is busy
5173 Ready import
5174 Imported cartridge has a duplicate barcode (%s) with slot %d - exporting it
5175 Remote sam not found
5176 Library full
5177 Move cartridge: from %d(%s) to %d: time %d seconds
5178 Move cartridge: from %d(%s) to %d: time %d seconds
5179 Move cartridge complete: from %d(%s) to %d
5180 Exchange media: from %d(%s) to %d to %d(%s): time %d seconds
5181 open(%s)
5182 Unable to open catalog %s 
5183 mmap(%s)
5184 Library reports %d drives, mcf has %d
5185 Library reports %d drives, mcf has %d
5186 Library reports no access to drive %d
5187 Unable to unload drive
5188 fstat(%s)
5189 Check mailbox ready
5190 Mail box sense %#x asc/acq %#x/%#x
5191 Mail box sense %#x asc/acq %#x/%#x
5192 Mail box sense %#x asc/acq %#x/%#x
5193 Door closed
5194 Door closed sense %#x asc/acq %#x/%#x
5195 Check wait_for_iport
5196 About to exchange with open fd
5197 Abnormal condition: VSN %s was no longer located at slot %d
5198 Exchange media:  from %d(%s) to %d to %d(%s): time %d seconds
5199 Source slot %d is unavaiable
5200 Destination slot %d is unavaiable
5201 Read Element Status failed - retries exhausted
5202 Door Lock command fails - retries exhausted
5203 Move Medium command fails - retries exhausted
5204 Position to Element command fails - retries exhausted
5205 Library did not become ready - retries exhausted
5206 Mode Select fails - retries exhausted
5207 Move cartridge from %d(%s) to %d fails - retries exhausted
5208 Exchange media from %d(%s) to %d to %d(%s) fails - retries exhausted
5209 Rezero unit fails - retries exhausted
5210 Cannot clear transport - Move medium fails - retries exhausted
5211 Cannot clear transport - Move medium fails - retries exhausted
5212 Cannot clear transport - Move medium fails - retries exhausted
5213 Initialize Element Status fails - retries exhausted
5214 Initialize Element Status fails - retries exhausted
5215 Initialize Element Status fails - retries exhausted
5216 Cleaning has not completed in the allocated time
5217 Mode Sense fails - retries exhausted
5218 Spin Up Fails
5219 Init Element Range failed - retries exhausted
5220 Library element report: %d drive, %d transport, %d storage, %d import/export
5221 Mode sense for page %#x failed
5222 Mode select for page %#x failed
5223 Operator panel lock status: %d
5230 Bus device reset occurred while staging, decreasing optical buffer to %d
5301 TUR failure: Slot %d: No media
5302 TUR failure: Slot %d: Needs formatting
5303 TUR failure: Slot %d: Unit attention error
5304 TUR failure: Slot %d: Hardware error
5305 TUR failure: Slot %d: Bad media
5306 TUR failure: Slot %d: Illegal request
5307 TUR failure: Slot %d: Unknown error (%d/%d)
5308 TUR failure: Slot %d: Aborted Command
5309 TUR failure: Slot %d: Sense Key %#x ASC %#x ASCQ %#x
5310 TUR failure: Slot %d: Failed to get the device ready for access: Retries exceeded
5330 SC: dev %d  cmd %#x
5331 Drive returns : needs formatting
5332 Timeout on Clear drive [duration %d secs, active state %d]: Failed to remove slot %d from drive
5333 Cannot determine which drive is bad [%d, %d, %d]
5334 Cleaning cycle complete : %d cycles left
5335 ** NOT USED ** Read storage element status failed: Slot %d in drive.
5336 Slot %d free in storage element, inuse in catalog
5337 No free slot
5338 Found free slot %d
5339 Drive %d slot %d occupied
5340 Transport slot %d occupied
5341 Imp/Exp slot %d occupied
5342 Move empty media tray: from %d to %d
5343 Move empty media tray failed
5344 Move empty media tray complete
5345 Position removable media file from 0x%x to 0x%x
5346 init_library_configuration failed
5347 TUR failure: No media
5348 TUR failure: Needs formatting
5349 TUR failure: Unit attention error
5350 TUR failure: Hardware error
5351 TUR failure: Bad media
5352 TUR failure: Illegal request
5353 TUR failure: Unknown error (%d/%d)
5354 TUR failure: Aborted Command
5355 TUR failure: Sense Key %#x ASC %#x ASCQ %#x
5356 TUR failure: Retries exceeded
5357 Cannot clear drive : move failed
5358 Cannot clear drive: move failed
5359 Timeout on Clear drive [duration %d secs, active state %d]
5360 ** NOT USED ** Read storage element status failed
5361 Library element reports no drives available
5362 %d drives available
5363 Drive exception bit set (%#x:%#x)
5364 Clean failed, drive remains dirty after cleaning
5365 position_rmedia locking active_io setpos %llx, fd_stage %d
5366 dodirio locking active_io setpos %llx, fd_stage %d
5367 Process scsi error, action %d

6000 Grau
6001 Slot %d: %s fails : d_errno %d (0x%x) %s
6002 Command fails : d_error %d: Range error 0-%d
6003 Move failed - cannot clear: Can not remove slot %d from drive
6004 Robot Audit not supported for GRAU
6005 ***
6006 ***
6007 ***
6008 aci_force_media: from %s
6009 Return from force: %s.
6010 force returned: status = %d: d_error = %d [%s]
6011 force return: call error %s
6012 to %s
6013 api returns: %s
6014 errno %d, d_error %d [%s]
6015 Call error: %s
6016 from %s
6017 api returns: %s
6018 errno %d, d_error %d [%s]
6019 Call error: %s
6020 from %s to %s
6021 api returns: %s
6022 errno %d, d_error %d [%s]
6023 Call error: %s
6024 Barcode %s
6025 api returns: %s
6026 errno %d, d_error %d [%s]
6027 Call error: %s
6028 VSN %s already mounted
6029 VSN %s
6030 VSN %s slot not occupied
6031 ***
6032 d_errno(%d): Range error 0-%d.
6033 VSN %s unload failed
6034 ***
6035 ***
6036 VSN %s mount failed
6037 Abnormal condition: VSN %s was no longer located at slot %d
6038 Thread failed for command %d, error %d
6039 Retries exceeded on %s
6040 API call for %s fails
6041 Add to catalog failed: %s
6042 default_capacity set to %d
6043 %s fails : d_errno %d (0x%x) %s
6044 Move failed - cannot clear


7000 IBM3494

8000 Stk
8001 Unable to mount on drive %d, drive is busy
8002 ACSLS cleaning cartridge unload check for complete remaining=%d
8003 Stk library auto-clean enabled, sam-fs cleaning cancelled 

9000 Migration

10000 Historian
10001 Medium %s is loaded in drive %d
10002 Medium %s is not available for %s
10003 open(%s)
10004 fstat(%s)
10005 mmap(%s)
10006 Catalog size inconsistent: is %d, expected %d
10007 open(%s)
10008 write(%s)
10009 VSN %s not found
10010 ** NOT USED ** Slot %d out of range
10011 Bad request %#x
10012 ** NOT USED ** Slot %d is not occupied
10013 VSN %s removed from historian
10014 ** NOT USED ** VSN %s already exists in catalog at slot %d
10015 ** NOT USED ** barcode already exists in catalog at slot %d
10016 Cc%cs(%d): %s-(%#x)%s
10017 Cc%ci(%d): %s
10018 Removing VSN %s %s
10019 CcRm: message area null
10020 MS:UpHp(%#x)
10021 Gc%cc(%d): %s(%#x)-%s
10022 Removing VSN %s %s
10023 CgRm(%d): message area null
10024 MS:GcHp(%#x)
10025 ftruncate(%s)
10026 mmap(%s)
10027 Waiting for 60 seconds
10028 thr_create(monitor_msg)
10029 thr_create(manage_list)
10030 SIGALRM: Thread(s) gone, exiting
10031 Unable to kill thread %d
10032 Bad internal event
10033 Bad magic %#x
10034 Unknown command %d
10035 Unknown event type %d
10036 thr_create(%s)
10037 thr_create(unknown)
10038 MS:MA: %#x
10039 MS:RV: %#x
10040 Shutdown request
10041 Adding VSN %s %s
10042 Cleaning tape successfully loaded in drive %d
10043 Stage request flags %x %x (canceled by fs)
10044 Stage request flags %x %x (handles match, open in progress)
10045 Stage request flags %d %d:%d.%d (handles match, open done)
10046 Error %d on write to media, catalog not updated.

11000 SEF
11001 Error getting supported log sense pages for device %s.
11002 Error getting log sense page %d for device %s.
11003 Unable to open sef log file, errno %d.
11004 Unable to malloc memory for sef data; skipping page %d for device %s.
11005 Error getting log sense page %d for device %s.
11006 Error writing log sense pages for dev %s, errno %d.
11010 Sef on.  Calling sefinit, device %s.
11011 Sef off.
11100 SEF supported
11101 Failed getting SEF page %x, %02x %02x %02x
11102 SEF sysevent failed at %s with error %s
11103 SEF get uints

12000 TapeAlert
12001 TapeAlert supported
12002 Failed getting TapeAlert %02x %02x %02x
12003 Invalid TapeAlert page length %d
12004 Invalid TapeAlert page code %x
12005 Invalid TapeAlert parameter code %x
12006 TapeAlert ver=%d eq=%d type=%d seq=%lld len=%d flags=0x%016llx
12007 TapeAlert ver=%d eq=%d type=%d seq=%lld len=%d flags=0x%016llx vsn=%s
12008 TapeAlert sysevent failed at %s with error %s
12009 TapeAlert mode sense current %02x %02x %02x
12010 TapeAlert mode sense changeable %02x %02x %02x
12011 TapeAlert mode select polling %02x %02x %02x
12012 TapeAlert mode sense updated %02x %02x %02x
12013 TapeAlert run-time page 0x1c bytes 2 and 3: %02x %02x
12014 TapeAlert local file descriptor open failed

13000 PlasmonG
13001 Plasmon G Enterprise emulation mode is not supported. Reconfigure the library.
13002 Plasmon G Enterprise addressing scheme 1 must be used. Reconfigure the library.
13003 Plasmon G Enterprise barcode type must be 2 or 3. Reconfigure the library.
