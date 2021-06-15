@echo off

rem
rem  This cmd file creates the libraries used for the bamsserver
rem  installation.  This file must remain in synch with the latest
rem  list of files needed to install.  This file must also remain
rem  in synch with the FILESnn.DAT files
rem

del Bamsserv
del Jobexec 
del RTFBase 
del RTFProd 
del CfgBase 
del CfgProd 
del JobExe1 
del JobExe2 
del JobExe3 
del JobDLL1 
del JobDLL2 
del JobDLL3 


EbLib -a Bamsserv bamsserv.exe
EbLib -a Jobexec  Jobs\JobExec.exe Jobs\RtfPrep.EXE
Eblib -a RTFBase  Jobs\Template\RTFBase\*.*
Eblib -a RTFProd  Jobs\Template\RTFProd\*.*
Eblib -a CfgBase  Jobs\CfgBase\*.*
Eblib -a CfgProd  Jobs\CfgProd\*.*

Eblib -a JobExe1  Jobs\EXE\
Eblib -a JobExe2  Jobs\EXE\
Eblib -a JobExe3  Jobs\EXE\

Eblib -a JobDLL1  Jobs\DLL\
Eblib -a JobDLL2  Jobs\DLL\
Eblib -a JobDLL3  Jobs\DLL\




dbsqlpaf.dll    48147   3-15-95 11:53a 
dbsqlpai.dll    35347   3-15-95 11:50a 
dbsqlpaj.dll    35347   3-15-95 11:48a 
dbsqlpat.dll    61459   3-23-95  9:14a 
dbsqlpau.dll    48147   3-12-95 10:36p 
dbsqlpb.dll     51218   3-23-95  9:26a 
dbsqlpbc.dll    42003   3-23-95  9:19a 
dbsqlpbd.dll    51219   3-23-95  9:25a 
dbsqlpbi.dll    41491   3-23-95  9:19a 
dbsqlpbl.dll    42003   3-23-95  9:23a 
dbsqlpbn.dll    39443   3-23-95  9:26a 
dbsqlpbp.dll    43539   3-23-95  9:20a 
dbsqlpc.dll     75794   3-23-95  9:04a 
dbsqlpcp.dll   114195   3-02-95 11:51a 
dbsqlpcu.dll    44563   3-23-95  9:20a 
dbsqlpcv.dll    36883   3-23-95  9:08a 
dbsqlpde.dll    36883   3-23-95  9:11a 
dbsqlpdi.dll    43027   3-15-95 11:49a 
dbsqlpdy.dll    37395   3-23-95  9:07a 
dbsqlpf.dll     49682   3-23-95  9:05a 
dbsqlpfu.dll    46611   3-23-95  9:24a 
dbsqlpga.dll    35859   3-23-95  9:19a 
dbsqlpi.dll     55826   3-23-95  9:05a 
dbsqlpil.dll    65555   3-23-95  9:06a 
dbsqlpio.dll    36883   3-23-95  9:07a 
dbsqlpld.dll    61459   3-15-95 11:56a 
dbsqlplp.dll    60947   3-23-95  9:23a 
dbsqlpp1.dll    88595   3-23-95  9:09a 
dbsqlpp2.dll    36883   3-23-95  9:10a 
dbsqlppc.dll    79891   3-02-95 11:54a 
dbsqlppd.dll    36371   3-15-95 11:48a 
dbsqlppe.dll    42003   3-23-95  9:03a 
dbsqlppf.dll    54291   3-02-95 11:53a 
dbsqlpph.dll    79379   3-23-95  9:21a 
dbsqlppi.dll    64531   3-23-95  9:15a 
dbsqlppj.dll    68115   3-02-95 11:52a 
dbsqlppl.dll    34835   3-23-95  9:18a 
dbsqlppo.dll    88595   3-23-95  9:02a 
dbsqlpps.dll    38931   3-23-95  9:08a 
dbsqlppt.dll    58899   3-02-95 11:55a 
dbsqlppv.dll    35347   3-23-95  9:13a 
dbsqlppw.dll    36371   3-02-95 11:56a 
dbsqlpre.dll    51219   3-23-95  9:11a 
dbsqlpri.dll    36883   3-23-95  9:07a 
dbsqlprs.dll    37395   3-23-95  9:11a 
dbsqlprv.dll    47123   3-23-95  9:13a 
dbsqlprx.dll    36883   3-23-95  9:17a 
dbsqlpva.dll    60435   3-23-95  9:22a 
dbsqlpvc.dll    49171   3-23-95  9:28a 
dbsqlpvn.dll    75283   3-23-95  9:21a 
dbsqlpvo.dll    57363   3-15-95 11:55a 
dbsqltai.dll    36371   3-15-95 11:50a 
dbsqltat.dll    54291   3-23-95  9:15a 
dbsqltb.dll     43538   3-23-95  9:26a 
dbsqltbl.dll    42515   3-23-95  9:23a 
dbsqltc.dll     66066   3-23-95  9:04a 
dbsqltcp.dll   100883   3-02-95 11:52a 
dbsqltdi.dll    43539   3-15-95 11:49a 
dbsqltfu.dll    39443   3-23-95  9:25a 
dbsqlti.dll     48146   3-23-95  9:05a 
dbsqltil.dll    56851   3-23-95  9:06a 
dbsqltlp.dll    52243   3-23-95  9:24a 
dbsqltp1.dll    77331   3-23-95  9:09a 
dbsqltp2.dll    37395   3-23-95  9:10a 
dbsqltpc.dll    70675   3-02-95 11:54a 
dbsqltpe.dll    43027   3-23-95  9:03a 
dbsqltpf.dll    46099   3-02-95 11:53a 
dbsqltpi.dll    56851   3-23-95  9:16a 
dbsqltpj.dll    59411   3-02-95  9:28p 
dbsqltpo.dll    78355   3-23-95  9:02a 
dbsqltpt.dll    50195   3-02-95 11:55a 
dbsqltrv.dll    39955   3-23-95  9:14a 
dbsqltvn.dll    65043   3-23-95  9:22a 
dbsqlty0.dll    37395   3-23-95  9:12a 
dbsqlty3.dll    40467   3-23-95  9:18a 
dbsqlty4.dll    40467   3-23-95  9:18a 
dbsqlty8.dll    34835   3-23-95  9:08a 
dbsqltz8.dll    48147   3-23-95  9:28a 
dbsqltz9.dll    35347   3-23-95  9:27a 
dbsqltza.dll    38931   3-21-95  8:52p 
dbsqltzc.dll    38931   3-21-95  4:21p 
dbsqltzg.dll    37907   3-23-95  9:25a 
dbsqltzi.dll    87059   3-02-95 11:57a 
dbsqltzj.dll   105491   3-02-95  8:44p 
dbsqltzk.dll    61971   3-02-95 11:59a 
dbsqltzl.dll    58387   3-02-95 11:58a 
dbsqltzm.dll    87571   3-02-95  9:04p 
dbsqltzn.dll    38931   3-23-95  9:16a 
dbsqltzo.dll    40467   3-23-95  9:16a 
dbsqltzp.dll    38931   3-23-95  9:17a 
dbsqltzq.dll    40467   3-23-95  9:17a 
dbsqltzx.dll    38931   3-23-95  9:12a 
dbsqltzy.dll    37395   3-23-95  9:13a 
dbsqltzz.dll    38931   3-23-95  9:12a 
dbsvcs.dll     175853   3-23-95 10:45a 
dbsvcs0.dll    167709   2-27-95 10:01p 



dcefchk.exe    763511   3-24-95  9:47a 
dcefund.exe    760224   3-24-95  9:48a 
dcelcst.exe    761891   3-24-95  9:47a 
dceload.exe    766332   3-24-95  9:47a 
ldletb.exe     667626   3-12-95  1:01p 
lgcscb.exe     688888   3-12-95 12:00p 
lgebtb.exe     639639   3-12-95  1:16p 
lglipb.exe     641546   3-12-95  1:16p 
lpdssb.exe     718025   3-12-95  1:20p 
lxtabb.exe     655460   3-12-95  1:16p 
peprjb.exe     648189   3-17-95 12:39p 
pgbbpb.exe     774782   3-21-95  3:19p 
pgpdeb.exe     678913   3-24-95  9:48a 
pxprjb.exe     669758   3-17-95 12:39p 
rtflist.exe    645680   3-23-95 10:51a 
serunple.cmd     2995   3-09-95  4:58p 
seruples.cmd     4385   3-23-95  6:29p 
stdlist.cmd      2864   3-02-95  2:43p 

