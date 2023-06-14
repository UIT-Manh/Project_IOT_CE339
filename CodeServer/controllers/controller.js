const environment = require('../model/environment');
var that = (module.exports = {
  mainPage: async (req, res, next) => {
    res.render(__dirname + "/../views/main.ejs");
  },
  addData: async(data) => {
    console.log(data)
    try
    { 
      const newEnv = new environment({
          temp : data.temp,
          hum: data.hum,
          time: data.time
      })
      await newEnv.save()
      console.log("Add data environment successfully!")
    }
    catch (err) {
        console.log("Create error :" + {message:err})
    }
  },
  getData30ls: async() => {
    try
    { 
      const newEnv = new environment({
          temp : data.temp,
          hum: data.hum,
          time: data.time
      })
      await newEnv.save()
      console.log("Add data environment successfully!")
    }
    catch (err) {
        console.log("Create error :" + {message:err})
    }
  }
});
