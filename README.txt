This project will implement a modular 450W DC programmable load. To keep things simple, it uses a 
single IXYS Linear MOSFET instead of paralleling many MOSFETs. Most modern MOSFETs are not very well
suited to linear operation and many datasheets do not even characterize the DC SOA. One of the 
reasons is that MOSFETs are generally composed of many parallel MOSFETs internally and this can result
in hot spotting. For a one off or hobbyist load, I think it is easier to pay (or sample) the much higher 
price for a few mosfets that are precharacterized and will reliably give you 450W of dissapation. The 
alternative would be to buy many opamps and mosfets to divide the load. Assuming that you run a typical
TO220 package at a conservative 10W, you would need to make sure to get the price per channel down to
around a dollar to make it cost competitive. Definitely achievable, but now you need to mount 45 TO220s.
The MOSFET I have chosen is easily water coolable and even already isolated. The control loop is formed 
by a very cool ADA4523-1 zero drift opamp. Its 4uV of offset means only 4mA of offset current using a 
1 milliohm current shunt. Zero drift is very important here since any temperature change will result in 
the offset current changing noticiably due to the low shunt resistances