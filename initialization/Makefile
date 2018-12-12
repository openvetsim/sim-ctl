# makefile
#

default: all

all: 

install: all
	sudo cp -u simctl /etc/init.d/simctl
	sudo chmod ug+x /etc/init.d/simctl
	# sudo insserv simctl
	sudo update-rc.d simctl defaults
	if [ ! -d /simulator ]; then \
		sudo mkdir /simulator; \
		sudo chown debian:debian /simulator; \
		cp rfid.xml /simulator; \
		cp soundList.csv /simulator; \
	fi
	
	if [ ! -f /simulator/soundList.csv ]; then \
		cp soundList.csv /simulator; \
	fi

clean:
 