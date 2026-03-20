
# Egis EG171 HAL

This  SDK is include of three components

- USB library\
    Provides related functions of USB device interface.

- Crypto library\
    Provides related functions of hw-hash and hw-trng.

- Others\
    Provides related functions of base OTP and SMU oprations.

---
---


# License Notice

### Notice of USB component

Copyright (c) 2025 Egis Technology Inc. All rights reserved.

This library is based on Cadence USB IP, licensed under the Cadence, Inc. Proprietary Software License and Maintenance Agreement and related Product Quotation (the “Agreement”) between Egis Technology Inc. and Cadence Design System Limited.
Modifications have been made by Egis Technology Inc.

Distribution of source code is restricted under the original license terms.
Only compiled binary (.a) and header files (.h) are provided for integration.

This license includes the following files:
```
.
└─ et171
　　　├─ inc
　　　│　　└─ et171_usb
　　　│　　　　　└─ *.h
　　　└─ lib
　　　　　　└─ libusbcore_*.a
```

##### License Grant:
Subject to the terms and conditions of this Agreement, Egis Technology Inc. is granted a non-exclusive, non-transferable license to use, modify, and integrate the Licensed IP solely for the purpose of developing and manufacturing authorized products.
Such activities may only be performed by Egis Technology Inc. and its designated subcontractors, strictly for assisting in design, development, and manufacturing of the licensed products.

##### Restrictions:
- Redistribution of source code is prohibited.
- Public disclosure of source code is prohibited.
- The Licensed IP may not be sublicensed, except as expressly permitted herein.
- The Licensed IP may not be used by any third party other than Egis Technology Inc. and its designated subcontractors for the purposes stated above.

##### NOTICE:
This product is NOT licensed under Apache-2.0 or BSD-3. It is proprietary and subject to the original license restrictions.


### Notice of Crypto component

Copyright (c) 2025 Egis Technology Inc. All rights reserved.

This library is based on Selix crypto IP, licensed under Silex Insight. the Proprietary License Agreement between Egis Technology Inc. and Silex Insight SA.
Modifications have been made by Egis Technology Inc.

Distribution of source code is restricted under the original license terms.
Only compiled binary (.a) and header files (.h) are provided for integration.

This license includes the following files:
```
.
└─ et171
　　　├─ inc
　　　│　　└─ sxsymcrypt
　　　│　　　　　└─ *.h
　　　└─ lib
　　　　　　└─ libsxsymcrypt_*.a
```

##### License Grant:
Subject to the terms and conditions of this Agreement, Egis Technology Inc. is granted a non-exclusive, non-transferable license to use, modify, and integrate the Licensed IP solely for the purpose of developing and manufacturing authorized products.
Such activities may only be performed by Egis Technology Inc. and its designated subcontractors, strictly for assisting in design, development, and manufacturing of the licensed products.

##### Restrictions:
- Redistribution of source code is prohibited.
- Public disclosure of source code is prohibited.
- The Licensed IP may not be sublicensed, except as expressly permitted herein.
- The Licensed IP may not be used by any third party other than Egis Technology Inc. and its designated subcontractors for the purposes stated above.

##### NOTICE:
This product is NOT licensed under Apache-2.0 or BSD-3. It is proprietary and subject to the original license restrictions.

### Notice of Others

Copyright (c) 2025 Egis Technology Inc. All rights reserved.

Distribution of source code is restricted under the original license terms.
Only compiled binary (.a) and header files (.h) are provided for integration.

This license includes the following files:
```
.
├─ et171
│　　├─ inc
│　　│　　└─ et171_hal
│　　│　　　　　└─ *.h
│　　└─ lib
│　　 　　└─ libet171hal_*.a
└─ zephyr
 　　└─ module.yml
```

---
---
