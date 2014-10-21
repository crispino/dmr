# The configure of Makefile
# You can modify the file, if you need.
#   Remove Config
#     eg:  !CONFIG_DLOG=yes

# audioplay
CONFIG_I2S=yes
CONFIG_RLOG=yes
CONFIG_SOFTWARE_VOLUME=yes

# lib configurations
CONFIG_FFMPEG=yes
CONFIG_FFMPEG_AAC_DECODER_FIXED=yes
CONFIG_FFMPEG_OGG_DECODER_FIXED=yes
CONFIG_FFMPEG_WMA_DECODER_FIXED=yes
!CONFIG_FAAD2=yes
!CONFIG_LIBOGG=yes
!CONFIG_TREMOR=yes