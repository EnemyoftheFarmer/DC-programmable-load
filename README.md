
## Modular Electronic DC Load

This project will implement a modular 450W DC programmable load. The motivation came from another 
project where I needed to measure the efficiency of a boost-buck converter. I needed a high power 
DC load FAST. The architecture is a straightfoward current sink. An opamp drives a MOSFET's gate input until the feedback voltage generated over a current shunt is equal to its set voltage which is derived from a DAC. For the typical DC load functions expected, such as constant power or constant resistance, a microcontroller will calculate the required DAC output voltage.



## Component Selection
#### MOSFET
To keep things simple, it uses a single IXYS IXTN200N10L2 Linear MOSFET instead of paralleling many MOSFETs. Most modern MOSFETs are not very well suited to linear operation and many datasheets do not even characterize the DC SOA. One of the reasons is that modern MOSFETs are generally composed of many parallel MOSFETs internally and this can result in hot spotting. The IXYS Linear MOSFETs are specifically built and tested to avoid this. For a one off or hobbyist 
load, I think it is easier to pay (or sample) the much higher price for a few mosfets that 
are precharacterized and will reliably give you 450W of dissapation. The alternative would be to
buy many opamps and mosfets to divide the load. Assuming that you run a typical
TO220 package at a conservative 10W, you would need to make sure to get the price per channel down to around a dollar to make it cost competitive. Definitely achievable, but now you need to mount 45 TO220s. The MOSFET I have chosen is easily water coolable and even already isolated. 
However, I am investigating alternatives since when this project originally started before the silicon shortage, the MOSFET could be bought for a hair under 30$ on mouser. It now costs around 50$... Luckily for the individual it can be sampled so if you are a student or hobbyist, you may be in luck and won't have to pay a dime if you head over to littlefuse's sample center. It may also be possible to buy directly from littlefuse for a far lower price. One alternative so far is the FDL100N50F, which has a DC SOA of 2.5kW. To me it seems a little unbelievable to get so much power dissapation in a TO-264 package but who knows. It does have an claimed incredible thermal resistance of just 0.05K/W. The good news is that it only costs 10$ per device and if we run it a very conservative 1/5 of claimed SOA, we still get 500W of power. The biggest downside to me is no screw terminals and no isolation. 
#### OPAMP 
The control loop is formed by a very cool ADA4523-1 zero drift opamp. Its 4uV of offset means only 4mA of offset current using a 1 milliohm current shunt. Zero drift is very important here since any temperature change will result in the offset current changing noticiably due to the low shunt resistance. It also has a very low 88nV p-p 0.1 to 10Hz input noise which is pretty much all we care about in our DC load anyways. This begs the question why we need a low noise percision op amp for a DC load and the answer is I wanted to be able to set mAs on a 1 milliohm shunt. We need a 1 milliohm shunt because I wanted to sink at least 40A which would mean a 2.5W dissapted in the shunt resistor. An alternative but more expensive part is OPA189.   
#### DAC
My requirement to set mA to A in one range meant that I needed better than the typical 12 bit DAC. I also wanted it to be bipolar in case I needed to set negative voltage to calibrate out the OPAMP offset. In the first revision I did this by using a second DAC channel to generate an offset. But since I needed to replace that one anyways I chose the lazy method that also simplified the design. I ended up going with a AD5761R because it was the cheapest, 16bit, bipolar, TSSOP, and available to sample from Analog Devices DAC.
#### Microcontroller
For the brains of the system I went with a raspberry PICO. The first most important reason to me is I wanted to not use Arduino for once and I thought the documentation was very good. Addtionally, its cheap, fast, and wasn't hit by the silicon shortage. I didn't want to integrate an STM32 or the like knowing that no one else could reimplement this system because of supply disruption. 
#### ADC
I am using an ADS1115. Its classic, 16 bit and I had it laying around. My only question is if I should replace it with an ADS1118 so I can eliminate one bus and move completely to SPI.
## Cooling
The cooling system consists primarly out of PC water cooling parts. The radiator is 240mm by 40mm. I just slapped two 120mm fans I had lying around on it. The resovoir and pump are also just cheap amazon PC water cooling components. In my test runs so far, I seem to be to able to keep the mosfet at around 65 degrees while under 450W of load with the fans at full tilt after the water hits saturation. That puts the junction temp. at ~133 degrees. While still under the TJ_MAX of 150 degrees, I am not sure how this impacts the lifespan of the device, especially considering its high price. Either the cooling capacity needs to be upgraded, the load run less hard, or tests/enquires made to see if its something to worry about  There are tentative plans to attach a thermister to the MOSFET and write some code to make sure that TJ_MAX is never exceeded. 
 
## Prototype Photo
![alt text](https://github.com/[username]/[reponame]/blob/[branch]/image.jpg?raw=true)