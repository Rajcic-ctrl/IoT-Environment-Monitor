using IOTProjekat.DTOs;
using IOTProjekat.Services;
using Microsoft.AspNetCore.Mvc;

namespace IOTProjekat.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class SensorController : ControllerBase
    {
        private readonly ISensorService _sensorService;
        public SensorController(ISensorService sensorService)
        {
            _sensorService = sensorService;
        }
        [HttpPost]
        public async Task<IActionResult> AddSensorData([FromBody] AddSensorDataDto dto)
        {
            await _sensorService.AddData(dto);

            return Ok(dto);
        }
        [HttpGet("all")]
        public async Task<IActionResult> GetAllData()
        {
            var data = await _sensorService.GetAll();

            return Ok(data);
        }
        [HttpGet("latest")]
        public async Task<IActionResult> GetLatest()
        {
            var data = await _sensorService.GetLatest();

            return Ok(data);
        }
    }
}
